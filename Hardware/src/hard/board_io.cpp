#include "board_io.hpp"

#include <string.h>
#include <unistd.h>
#include <string>

board_io::board_io()
{
	this->gpio_control = std::unique_ptr<gpio>(new gpio());
	this->spi_control = std::unique_ptr<spi_dma>(new spi_dma());;

	//出力モード設定
	this->gpio_control->mode_write(GPIO_SELECT, GPIO_OUTPUT);
	this->gpio_control->mode_write(GPIO_TFT_RESET, GPIO_OUTPUT);
	this->gpio_control->mode_write(GPIO_TFT_LED, GPIO_OUTPUT);

	//SPIポートの出力設定
	this->gpio_control->mode_write(3, GPIO_ALT0); //CE1
	this->gpio_control->mode_write(4, GPIO_ALT0); //CE0
	this->gpio_control->mode_write(5, GPIO_ALT0); //MISO
	this->gpio_control->mode_write(6, GPIO_ALT0); //MOSI
	this->gpio_control->mode_write(7, GPIO_ALT0); //SCLK


	//出力の初期化
	this->gpio_control->data_write(GPIO_SELECT, HIGH);
	this->gpio_control->data_write(GPIO_TFT_RESET, HIGH);
	this->gpio_control->data_write(GPIO_TFT_LED, HIGH);

	usleep(10);

	//高速化のためあらかじめ定義
	this->th_dat_info =
		std::unique_ptr<spi_dma_info_container>(new spi_dma_info_container(GPIO_SPI_SELECT_TOUCH, TOUCH_RW_SPEED, SPI_DMA_SYNC));
	this->tr_com_info = 
		std::unique_ptr<spi_dma_info_container>(new spi_dma_info_container(GPIO_SPI_SELECT_TFT, TFT_READ_SPEED_NORMAL, SPI_DMA_SYNC));
	this->tw_com_info =
		std::unique_ptr<spi_dma_info_container>(new spi_dma_info_container(GPIO_SPI_SELECT_TFT, TFT_WRITE_SPEED_NORMAL, SPI_DMA_SYNC));
	this->tw_dat_info =
		std::unique_ptr<spi_dma_info_container>(new spi_dma_info_container(GPIO_SPI_SELECT_TFT, TFT_WRITE_SPEED_NORMAL, SPI_DMA_SYNC));
	this->wp_com_info =
		std::unique_ptr<spi_dma_info_container>(new spi_dma_info_container(GPIO_SPI_SELECT_TFT, TFT_WRITE_SPEED_NORMAL, SPI_DMA_SYNC));
	this->wp_pix_info =
		std::unique_ptr<spi_dma_info_container>(new spi_dma_info_container(GPIO_SPI_SELECT_TFT, TFT_WRITE_SPEED_FAST, SPI_DMA_SYNC));

	//TFTの初期化
	this->tft_reset();
}

board_io::~board_io()
{
}

void board_io::get_touch_pos(POINT* touch_pos)
{
	//DMA with SPIを利用してタッチパネル状態の取得

	for(int ch_select = 0; ch_select <= 1; ch_select++){
		//XかY切替出力
		this->gpio_control->data_write(GPIO_SELECT, ch_select);

		unsigned char data_tmp[2];
		data_tmp[0] = (0b1100 | (ch_select << 1)) << 4;
		data_tmp[1] = 0;

		this->th_dat_info->setDataContent(data_tmp, sizeof(data_tmp));
		this->spi_control->start(*(this->th_dat_info));

		this->th_dat_info->getDataContent(data_tmp, sizeof(data_tmp));

		unsigned short val = ((data_tmp[0] & 0xF) << 7) | (data_tmp[1] >> 1);

		switch(ch_select){
		case 0:	touch_pos->x = val; break;
		case 1: touch_pos->y = val; break;
		}
	}

	//XかY切替出力をHIGHに戻す
	this->gpio_control->data_write(GPIO_SELECT, HIGH);
}

void board_io::tft_read(unsigned char command, unsigned char* parameters, int param_length)
{
	//TFTコマンドパラメタ読み取り関数

	// D/CをLOWに
	this->gpio_control->data_write(GPIO_SELECT, LOW);

	this->tr_com_info->setDataLength(param_length + 1);
	this->tr_com_info->setDataContent(&command, 1);

	this->spi_control->start(*(this->tr_com_info));

	if(param_length > 0){
		unsigned char* param_tmp = new unsigned char[param_length + 1];

		this->tr_com_info->getDataContent(param_tmp, param_length + 1);
		memcpy(parameters, &param_tmp[1], param_length);

		delete param_tmp;
	}

	// D/CをHIGHに戻す
	this->gpio_control->data_write(GPIO_SELECT, HIGH);
}

void board_io::tft_write_command(unsigned char command, unsigned char* parameters, int param_length)
{
	//TFTのコマンド書き込み関数

	// D/CをLOWに
	this->gpio_control->data_write(GPIO_SELECT, LOW);

	this->tw_com_info->setDataContent(&command, 1);

	//コマンド書き込み
	this->spi_control->start(*(this->tw_com_info));

	// D/CをHIGHに
	this->gpio_control->data_write(GPIO_SELECT, HIGH);

	if(param_length > 0){
		//パラメタ書き込み
		this->tw_dat_info->setDataContent(parameters, param_length);

		this->spi_control->start(*(this->tw_dat_info));
	}
}

void board_io::tft_write_pixels(unsigned char command, unsigned char* pixels, int pixel_length)
{
	//TFTのピクセル書き込み関数

	//コマンド書き込み用準備
	this->wp_com_info->setDataContent(&command, 1);

	//ピクセル書き込み用準備
	this->wp_pix_info->setDataContent(pixels, pixel_length);

	//同期処理(ディスプレイ状態がONでない場合は待つ必要なし)
	if(this->display_status == DISPLAY_STATUS_ON){
		unsigned char ctmp[3];

		this->tft_read(0x45, ctmp, 3);
		unsigned short now_line = ((ctmp[0] & 1) << 9) | (ctmp[1] << 1) | ((ctmp[2] & 0b10000000) >> 7);

		//TFTのリフレッシュレート(615KHz / 27 / (320 + (127 + 64)))から算出：((1 / FPS / 512) * 1000 * 1000)
		//5000～3400 , + 4200は、(5000-3400)/2 + 3400の処理時間
		usleep((512 - now_line) * 43.81669 + 4200);
	}

	// D/CをLOWにして
	this->gpio_control->data_write(GPIO_SELECT, LOW);
	//コマンドを書き込む
	this->spi_control->start(*(this->wp_com_info));

	// D/CをHIGHに
	this->gpio_control->data_write(GPIO_SELECT, HIGH);
	//ピクセル書き込み
	this->spi_control->start(*(this->wp_pix_info));
}

void board_io::tft_reset()
{
	this->gpio_control->data_write(GPIO_TFT_RESET, LOW);
	usleep(2000);
	this->gpio_control->data_write(GPIO_TFT_RESET, HIGH);
	usleep(2000);

	//SetPixelFormat(6-6-6)
	unsigned char tmp = 0b01100110;
	this->tft_write_command(0x3A, &tmp, 1);

/*
	tmp = 0b1000;
	this->tft_write_command(0x36, &tmp, 1);
*/
	//SetAddress
	unsigned char address[4];
	//SetColumnAddress
	address[0] = 0;
	address[1] = 0;
	address[2] = 0;
	address[3] = 0xEF;
	this->tft_write_command(0x2A, address, sizeof(address));

	//SetPageAddress
	address[0] = 0;
	address[1] = 0;
	address[2] = 0x01;
	address[3] = 0x3F;
	this->tft_write_command(0x2B, address, sizeof(address));

	//ポーチ設定
	unsigned char porch_tmp[4];

	//縦フロントポーチ64、バックポーチ127
	porch_tmp[0] = 64;
	porch_tmp[1] = 127;
	porch_tmp[2] = 64;
	porch_tmp[3] = 64;
	this->tft_write_command(0xB5, porch_tmp, sizeof(porch_tmp));
}

void board_io::set_tft_status(int enable)
{
	if(enable == 0){
		this->gpio_control->data_write(GPIO_TFT_LED, HIGH);

		//Display OFF
		tft_write_command(0x28, NULL, 0);
		//Sleep IN
		tft_write_command(0x10, NULL, 0);

		this->display_status = DISPLAY_STATUS_OFF;
	}else{
		//Sleep OUT
		tft_write_command(0x11, NULL, 0);
		//Display ON
		tft_write_command(0x29, NULL, 0);

		//ディスプレイの応答を待つ
		int resp_flag = 0;
		unsigned char ctmp[3];
		for(int idx = 0; idx < 1000; idx++){
			this->tft_read(0x45, ctmp, 3);
			unsigned short now_line = ((ctmp[0] & 1) << 9) | (ctmp[1] << 1) | ((ctmp[2] & 0b10000000) >> 7);
			if(now_line > 0) {
				resp_flag = 1;
				break;
			}
			usleep(100);
		}
		if( !resp_flag) throw std::string("液晶ディスプレイから応答がありませんでした");

		this->gpio_control->data_write(GPIO_TFT_LED, LOW);

		this->display_status = DISPLAY_STATUS_ON;
	}
}
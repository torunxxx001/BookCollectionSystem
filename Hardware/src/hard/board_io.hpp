#ifndef __BOARD_IO_HPP__
#define __BOARD_IO_HPP__

#include "spi_dma.hpp"
#include "base.hpp"

#include <memory>


#define GPIO_SELECT 0
#define GPIO_TFT_RESET 1
#define GPIO_TFT_LED 2

#define GPIO_SPI_SELECT_TFT		SPI_DMA_CE1
#define GPIO_SPI_SELECT_TOUCH 	SPI_DMA_CE0

/* タッチ検出は125MHz/1024 = 122Kbpsがいい */
#define TOUCH_RW_SPEED 1024

/* readは 125MHz/16 = 7.8125Mbpsぐらいがちょうどいい */
#define TFT_READ_SPEED_NORMAL 16

//NORMALなら書き込み速度125MHz/16 = 7.8125Mbps
//FASTなら書き込み速度125MHz/4 = 31.25Mbps
#define TFT_WRITE_SPEED_NORMAL 	8
#define TFT_WRITE_SPEED_FAST	4

#define DISPLAY_STATUS_ON 1
#define DISPLAY_STATUS_OFF 0

//このクラスはカプセル化
class board_io {
private:
	std::unique_ptr<gpio> gpio_control;
	std::unique_ptr<spi_dma> spi_control;

	//高速化のための変数定義
	std::unique_ptr<spi_dma_info_container> th_dat_info;
	std::unique_ptr<spi_dma_info_container> tr_com_info;
	std::unique_ptr<spi_dma_info_container> tw_com_info;
	std::unique_ptr<spi_dma_info_container> tw_dat_info;
	std::unique_ptr<spi_dma_info_container> wp_com_info;
	std::unique_ptr<spi_dma_info_container> wp_pix_info;
	//変数定義終わり

	int display_status;
public:
	board_io();
	~board_io();

	void get_touch_pos(POINT* touch_pos);
	void tft_read(unsigned char command, unsigned char* parameters, int param_length);
	void tft_write_command(unsigned char command, unsigned char* parameters, int param_length);
	void tft_write_pixels(unsigned char command, unsigned char* pixels, int pixel_length);

	void tft_reset();
	void set_tft_status(int enable);
};

#endif

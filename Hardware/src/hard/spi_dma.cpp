#include "spi_dma.hpp"

#include <string>
#include <string.h>
#include <unistd.h>
#include "memory.h"

spi_dma_info_container::spi_dma_info_container(int chip_select, int speed_divider, int sync_flag)
{
	memset(&this->spi_dma_info, 0, sizeof(SPI_DMA_INFO));
	memset(&this->old_spi_dma_info, 0, sizeof(SPI_DMA_INFO));

	this->setConfig(chip_select, speed_divider, sync_flag);
	memcpy(&this->old_spi_dma_info, &this->spi_dma_info, sizeof(SPI_DMA_INFO));
}

spi_dma_info_container::~spi_dma_info_container()
{
	this->free();
}

void spi_dma_info_container::alloc()
{
	if(this->spi_dma_info.length <= 0) return;

	//送信データ用バッファ
	this->spi_dma_info.write_data = (unsigned char*)memalloc(this->spi_dma_info.length);
	//受信データ用バッファ
	this->spi_dma_info.read_data = (unsigned char*)memalloc(this->spi_dma_info.length);

	//バッファをクリアしておく
	memset(this->spi_dma_info.write_data, 0, this->spi_dma_info.length);
	memset(this->spi_dma_info.read_data, 0, this->spi_dma_info.length);

	//DMA内SPIコントロール情報用
	this->spi_dma_info.spicon_length = CEIL(this->spi_dma_info.length / 61440.0) * sizeof(unsigned int);
	this->spi_dma_info.spicon = (unsigned int*)memalloc(this->spi_dma_info.spicon_length);

	int dummy_num = this->spi_dma_info.speed_divider * 8;

	//DMAコントロールブロック
	this->spi_dma_info.dma_tx_cb_size = (CEIL(this->spi_dma_info.length / 4096.0) + CEIL(this->spi_dma_info.length / 61440.0) 
								+ (CEIL(this->spi_dma_info.length / 61440.0) - 1) * dummy_num) * sizeof(dma_cb_t);
	this->spi_dma_info.dma_tx_cb = (dma_cb_t*)memalloc(this->spi_dma_info.dma_tx_cb_size);

	this->spi_dma_info.dma_rx_cb_size = CEIL(this->spi_dma_info.length / 4096.0) * sizeof(dma_cb_t);
	this->spi_dma_info.dma_rx_cb = (dma_cb_t*)memalloc(this->spi_dma_info.dma_rx_cb_size);

	dma_cb_t spi_con_cb_template;
	dma_cb_t dummy_cb_template;
	dma_cb_t tx_cb_template;
	dma_cb_t rx_cb_template;

	spi_con_cb_template.ti = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP;
	spi_con_cb_template.source_ad = 0;
	spi_con_cb_template.dest_ad = SPI_FIFO_PHYSIC_ADDR;
	spi_con_cb_template.txfr_len = 4;
	spi_con_cb_template.stride = 0;
	spi_con_cb_template.nextconbk = 0;

	dummy_cb_template.ti = DMA_WAITS(15);
	dummy_cb_template.source_ad = 0;
	dummy_cb_template.dest_ad = 0;
	dummy_cb_template.txfr_len = 0;
	dummy_cb_template.stride = 0;
	dummy_cb_template.nextconbk = 0;

	tx_cb_template.ti = DMA_NO_WIDE_BURSTS | DMA_SRC_INC | DMA_PERMAP(6) | DMA_DEST_DREQ | DMA_WAIT_RESP;
	tx_cb_template.source_ad = 0;
	tx_cb_template.dest_ad = SPI_FIFO_PHYSIC_ADDR;
	tx_cb_template.txfr_len = 4096;
	tx_cb_template.stride = 0;
	tx_cb_template.nextconbk = 0;

	rx_cb_template.ti = DMA_NO_WIDE_BURSTS | DMA_DEST_INC | DMA_PERMAP(7) | DMA_SRC_DREQ | DMA_WAIT_RESP;
	rx_cb_template.source_ad = SPI_FIFO_PHYSIC_ADDR;
	rx_cb_template.dest_ad = 0;
	rx_cb_template.txfr_len = 4096;
	rx_cb_template.stride = 0;
	rx_cb_template.nextconbk = 0;

	int remain = this->spi_dma_info.length;
	int sumlen = 0;
	int tx_cb_ptr = 0;
	int rx_cb_ptr = 0;
	int dma_block_cnt = 0;
	do{
		//ブロックの切れ目か
		if(sumlen % 61440 == 0){
			if(dma_block_cnt > 0){
				//ダミーブロックを入れる
				//SPI速度によってダミーの数を変更する
				for(int i = 0; i < dummy_num; i++){
					memcpy(&this->spi_dma_info.dma_tx_cb[tx_cb_ptr], &dummy_cb_template, sizeof(dma_cb_t));
					this->spi_dma_info.dma_tx_cb[tx_cb_ptr].nextconbk = conv_virt_to_physic(&this->spi_dma_info.dma_tx_cb[tx_cb_ptr + 1]);

					tx_cb_ptr++;
				}
			}

			//コントロール情報記述
			if(remain - 61440 >= 0){
				this->spi_dma_info.spicon[dma_block_cnt] = SPI_TA | SPI_SET_CS(this->spi_dma_info.chip_select) | (61440 << 16);
			}else{
				this->spi_dma_info.spicon[dma_block_cnt] = SPI_TA | SPI_SET_CS(this->spi_dma_info.chip_select) | (remain << 16);
			}

			//SPIコントロール用情報
			memcpy(&this->spi_dma_info.dma_tx_cb[tx_cb_ptr], &spi_con_cb_template, sizeof(dma_cb_t));

			this->spi_dma_info.dma_tx_cb[tx_cb_ptr].source_ad = conv_virt_to_physic(&this->spi_dma_info.spicon[dma_block_cnt]);
			this->spi_dma_info.dma_tx_cb[tx_cb_ptr].nextconbk = conv_virt_to_physic(&this->spi_dma_info.dma_tx_cb[tx_cb_ptr + 1]);

			tx_cb_ptr++;
			dma_block_cnt++;
		}

		//実データ用DMAコントロールブロック記述
		memcpy(&this->spi_dma_info.dma_tx_cb[tx_cb_ptr], &tx_cb_template, sizeof(dma_cb_t));
		memcpy(&this->spi_dma_info.dma_rx_cb[rx_cb_ptr], &rx_cb_template, sizeof(dma_cb_t));

		if(remain - 4096 <= 0){
			this->spi_dma_info.dma_tx_cb[tx_cb_ptr].txfr_len = remain;
			this->spi_dma_info.dma_rx_cb[rx_cb_ptr].txfr_len = remain;
		}else{
			this->spi_dma_info.dma_tx_cb[tx_cb_ptr].nextconbk = conv_virt_to_physic(&this->spi_dma_info.dma_tx_cb[tx_cb_ptr + 1]);
			this->spi_dma_info.dma_rx_cb[rx_cb_ptr].nextconbk = conv_virt_to_physic(&this->spi_dma_info.dma_rx_cb[rx_cb_ptr + 1]);
		}

		this->spi_dma_info.dma_tx_cb[tx_cb_ptr].source_ad = conv_virt_to_physic(this->spi_dma_info.write_data + sumlen);
		this->spi_dma_info.dma_rx_cb[rx_cb_ptr].dest_ad = conv_virt_to_physic(this->spi_dma_info.read_data + sumlen);

		sumlen += 4096;
		remain -= 4096;

		tx_cb_ptr++;
		rx_cb_ptr++;
	}while(remain > 0);
}

void spi_dma_info_container::free()
{
	if(this->spi_dma_info.write_data != NULL)
		memfree(this->spi_dma_info.write_data, this->spi_dma_info.length);
	if(this->spi_dma_info.read_data != NULL)
		memfree(this->spi_dma_info.read_data, this->spi_dma_info.length);
	if(this->spi_dma_info.spicon != NULL)
		memfree(this->spi_dma_info.spicon, this->spi_dma_info.spicon_length);

	if(this->spi_dma_info.dma_tx_cb != NULL)
		memfree(this->spi_dma_info.dma_tx_cb, this->spi_dma_info.dma_tx_cb_size);
	if(this->spi_dma_info.dma_rx_cb != NULL)
		memfree(this->spi_dma_info.dma_rx_cb, this->spi_dma_info.dma_rx_cb_size);
}

void spi_dma_info_container::realloc()
{
	SPI_DMA_INFO info_tmp;

	memcpy(&info_tmp, &this->spi_dma_info, sizeof(SPI_DMA_INFO));
	memcpy(&this->spi_dma_info, &this->old_spi_dma_info, sizeof(SPI_DMA_INFO));
	this->free();

	memcpy(&this->spi_dma_info, &info_tmp, sizeof(SPI_DMA_INFO));
	this->alloc();

	memcpy(&this->old_spi_dma_info, &this->spi_dma_info, sizeof(SPI_DMA_INFO));
}

void spi_dma_info_container::setConfig(int chip_select, int speed_divider, int sync_flag)
{
	if(chip_select != 0 && chip_select != 1){
		throw(std::string("ChipSelectは０か１にしてください"));
	}
	if(speed_divider <= 0 || speed_divider % 2){
		throw(std::string("SpeedDividerは２の倍数にしてください"));
	}

	this->spi_dma_info.chip_select = chip_select;
	this->spi_dma_info.speed_divider = speed_divider;
	this->spi_dma_info.sync_flag = sync_flag;

	this->realloc();
}

void spi_dma_info_container::setDataLength(int byte_length)
{
	if(byte_length <= 0) return;

	if(this->spi_dma_info.length != byte_length){
		this->spi_dma_info.length = byte_length;

		this->realloc();
	}
}

void spi_dma_info_container::setDataContent(void* indata, int byte_length)
{
	if(byte_length <= 0) return;

	if(this->spi_dma_info.length < byte_length){
		this->setDataLength(byte_length);
	}

	if(indata != NULL){
		memcpy(this->spi_dma_info.write_data, indata, byte_length);
	}else{
		memset(this->spi_dma_info.write_data, 0, byte_length);
	}
}

int spi_dma_info_container::getDataContent(void* outdata, int byte_limit)
{
	if(byte_limit <= 0) return 0;
	if(this->spi_dma_info.length < byte_limit){
		byte_limit = this->spi_dma_info.length;
	}

	memcpy(outdata, this->spi_dma_info.read_data, byte_limit);

	return byte_limit;
}

void spi_dma_info_container::copyInfo(SPI_DMA_INFO* info)
{
	memcpy(info, &this->spi_dma_info, sizeof(SPI_DMA_INFO));
}

volatile unsigned int* spi_dma::dma_control = NULL;
volatile unsigned int* spi_dma::spi_control = NULL;
int spi_dma::instance_count = 0;
int spi_dma::is_running = 0;

spi_dma::spi_dma()
{
	this->gpio_control = std::unique_ptr<gpio>(new gpio());

	if(this->instance_count == 0){
		this->dma_control = (volatile unsigned int*)map_physic(DMA_BASE, DMA_SIZE);
		this->spi_control = (volatile unsigned int*)map_physic(SPI_BASE, SPI_SIZE);

		if(this->dma_control == NULL){
			throw std::string("DMAレジスタのマッピングに失敗しました");
		}
		if(this->spi_control == NULL){
			throw std::string("SPIレジスタのマッピングに失敗しました");
		}
	}
	this->instance_count++;
}

spi_dma::~spi_dma()
{
	this->instance_count--;
	if(this->instance_count == 0){
		unmap_physic((void*)this->dma_control, DMA_SIZE);
		unmap_physic((void*)this->spi_control, SPI_SIZE);
	}
}

void spi_dma::start(spi_dma_info_container &info)
{
	SPI_DMA_INFO info_tmp;
	info.copyInfo(&info_tmp);

	if(info_tmp.length <= 0) return;

	volatile unsigned int* dma_tx = this->dma_control + DMA_CH13_OFFSET;
	volatile unsigned int* dma_rx = this->dma_control + DMA_CH14_OFFSET;

	for(int cnt = 0; cnt < SYNC_TIME_OUT; cnt++){
		if(spi_dma::is_running == 0 && !(dma_tx[DMA_CS] & DMA_ACTIVE) && !(dma_rx[DMA_CS] & DMA_ACTIVE)){
			break;
		}

		usleep(1000);
	}

	spi_dma::is_running = 1;

	//SPIのFIFOをクリア
	this->spi_control[SPI_CS] |= SPI_CLEAR_TX_FIFO | SPI_CLEAR_RX_FIFO;

	//SPI設定
	this->spi_control[SPI_CS] = SPI_AUTO_CS | SPI_DMAEN;
	this->spi_control[SPI_CLK] = SPI_CDIV(info_tmp.speed_divider);

	//DMAはリセットが必要
	dma_tx[DMA_CS] = DMA_RESET;
	dma_rx[DMA_CS] = DMA_RESET;

	//DMA Control Blockへのアドレスを指定
	dma_tx[DMA_CONBLK_AD] = conv_virt_to_physic(info_tmp.dma_tx_cb);
	dma_rx[DMA_CONBLK_AD] = conv_virt_to_physic(info_tmp.dma_rx_cb);

	//DMA有効化
	dma_rx[DMA_CS] = DMA_SET_PRIORITY(4) | DMA_WAIT_FOR_OUTSTANDING_WRITES | DMA_DISDEBUG | DMA_ACTIVE;
	dma_tx[DMA_CS] = DMA_SET_PRIORITY(4) | DMA_WAIT_FOR_OUTSTANDING_WRITES | DMA_DISDEBUG | DMA_ACTIVE;

	//DMA Engine1,2有効化
	*(this->dma_control + DMA_ENABLE) |= (1 << 13) | (1 << 14);

	if(info_tmp.sync_flag){
		//待ち時間は、転送サイズ / (250MHz / 8 / speed_divider)
		usleep(info_tmp.length / (31.25 / info_tmp.speed_divider));
	}

	spi_dma::is_running = 0;
}
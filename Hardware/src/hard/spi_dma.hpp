#ifndef __SPI_DMA_HPP__
#define __SPI_DMA_HPP__

#include "rpi_gpio.hpp"

#include <memory>


#define CEIL(x) (int)((int)(x) < (x) ? (x+1):(x))
#define ROUND_TO_PAGE(size) (((size) + 4096 - 1) & ~(4096 - 1))

#define DMA_BASE 0x20007000
#define DMA_SIZE 0xFF4

#define DMA_INT_STATUS	(0xFE0 / 4)
#define DMA_ENABLE		(0xFF0 / 4)

#define DMA_CH0_OFFSET	(0x000 / 4)
#define DMA_CH1_OFFSET	(0x100 / 4)
#define DMA_CH2_OFFSET	(0x200 / 4)
#define DMA_CH3_OFFSET	(0x300 / 4)
#define DMA_CH4_OFFSET	(0x400 / 4)
#define DMA_CH5_OFFSET	(0x500 / 4)
#define DMA_CH6_OFFSET	(0x600 / 4)
#define DMA_CH7_OFFSET	(0x700 / 4)
#define DMA_CH8_OFFSET	(0x800 / 4)
#define DMA_CH9_OFFSET	(0x900 / 4)
#define DMA_CH10_OFFSET	(0xA00 / 4)
#define DMA_CH11_OFFSET (0xB00 / 4)
#define DMA_CH12_OFFSET (0xC00 / 4)
#define DMA_CH13_OFFSET (0xD00 / 4)
#define DMA_CH14_OFFSET (0xE00 / 4)

#define DMA_CS			(0x00 / 4)
#define DMA_CONBLK_AD	(0x04 / 4)
#define DMA_DEBUG		(0x20 / 4)

//DMA CS Settings
#define DMA_ACTIVE		1
#define DMA_END			(1 << 1)
#define DMA_INT			(1 << 2)
#define DMA_SET_PRIORITY(val)		(val << 16)
#define DMA_SET_PANIC_PRIORITY(val)	(val << 20)
#define DMA_WAIT_FOR_OUTSTANDING_WRITES (1 << 28)
#define DMA_DISDEBUG	(1 << 29)
#define DMA_ABORT		(1 << 30)
#define DMA_RESET		(1 << 31)

//DMA TI Settings
#define DMA_NO_WIDE_BURSTS		(1 << 26)
#define DMA_WAITS(val)			(val << 21)
#define DMA_PERMAP(val)			(val << 16)
#define DMA_BURST_LENGTH(val)	(val << 12)
#define DMA_SRC_IGNORE			(1 << 11)
#define DMA_SRC_DREQ			(1 << 10)
#define DMA_SRC_WIDTH			(1 << 9)
#define DMA_SRC_INC				(1 << 8)
#define DMA_DEST_IGNORE			(1 << 7)
#define DMA_DEST_DREQ			(1 << 6)
#define DMA_DEST_WIDTH			(1 << 5)
#define DMA_DEST_INC			(1 << 4)
#define DMA_WAIT_RESP			(1 << 3)
#define DMA_TDMODE				(1 << 1)
#define DMA_INTEN				1

//DMA DEBUG Values
#define DMA_READ_LAST_NOT_SET_ERROR	1
#define DMA_FIFO_ERROR				(1 << 1)
#define DMA_READ_ERROR				(1 << 2)


//SPI Control
#define SPI_BASE		0x20204000
#define SPI_SIZE		0x18

#define SPI_CS			(0x00 / 4)
#define SPI_FIFO		(0x04 / 4)
#define SPI_CLK			(0x08 / 4)
#define SPI_DLEN		(0x0C / 4)
#define SPI_LTOH		(0x10 / 4)
#define SPI_DC			(0x14 / 4)

//SPI CS Values
#define SPI_LEN_LONG		(1 << 25)
#define SPI_DMA_LOSSI		(1 << 24)
#define SPI_CSPOL2			(1 << 23)
#define SPI_CSPOL1			(1 << 22)
#define SPI_CSPOL0			(1 << 21)
#define SPI_LOSSI			(1 << 13)
#define SPI_READ_ONLY		(1 << 12)
#define SPI_AUTO_CS			(1 << 11)
#define SPI_INTRXR			(1 << 10)
#define SPI_INTDONE			(1 << 9)
#define SPI_DMAEN			(1 << 8)
#define SPI_TA				(1 << 7)
#define SPI_CSPOL			(1 << 6)
#define SPI_CLEAR_TX_FIFO	(1 << 4)
#define SPI_CLEAR_RX_FIFO	(1 << 5)
#define SPI_CPOL			(1 << 3)
#define SPI_CPHA			(1 << 2)
#define SPI_SET_CS(val)		(val & 3)

//SPI CLK Values
#define SPI_CDIV(val)		(val & 0xFFFF)

//SPI DLEN Values
#define SPI_LEN(val)		(val & 0xFFFF)

//SPI LTOH Values
#define SPI_TOH(val)		(val & 0b111)

//SPI DMA DREQ Threshold
#define SPI_RPANIC(val)		(val << 24)
#define SPI_RDREQ(val)		(val << 16)
#define SPI_TPANIC(val)		(val << 8)
#define SPI_TDREQ(val)		(val)


#define GPSET_PHYSIC_ADDR	0x7E20001C
#define GPCLR_PHYSIC_ADDR	0x7E200028

#define SPI_CS_PHYSIC_ADDR		0x7E204000
#define SPI_FIFO_PHYSIC_ADDR	0x7E204004

#define SDRAM_PHSIC_ADDR	0x40000000


//設定用
#define SYNC_TIME_OUT 100000000

#define SPI_DMA_CE0 0
#define SPI_DMA_CE1 1
#define SPI_DMA_SYNC 1
#define SPI_DMA_ASYNC 0

//DMA設定用構造体
#pragma pack(push, 1)
typedef struct {
	unsigned int ti;
	unsigned int source_ad;
	unsigned int dest_ad;
	unsigned int txfr_len;
	unsigned int stride;
	unsigned int nextconbk;
	unsigned int dummy[2];
} dma_cb_t;
#pragma pack(pop)

//SPI操作用情報を格納するクラス
//注意:speed divider は最小４
typedef struct {
	//送受信基本情報
	int chip_select;
	int speed_divider;
	int sync_flag;
	int length;
	//自動セット
	unsigned char* write_data;
	unsigned char* read_data;

	//直接操作しない事
	dma_cb_t* dma_tx_cb;
	dma_cb_t* dma_rx_cb;
	int dma_tx_cb_size;
	int dma_rx_cb_size;

	unsigned int* spicon;
	int spicon_length;
} SPI_DMA_INFO;

//SPI操作用情報コンテナ
class spi_dma_info_container {
private:
	SPI_DMA_INFO spi_dma_info;
	SPI_DMA_INFO old_spi_dma_info;

	void alloc();
	void free();
	void realloc();

public:
	spi_dma_info_container(int chip_select, int speed_divider, int sync_flag);
	~spi_dma_info_container();

	void setConfig(int chip_select, int speed_divider, int sync_flag);
	void setDataLength(int byte_length);
	void setDataContent(void* indata, int byte_length);
	int getDataContent(void* outdata, int byte_limit);

	void copyInfo(SPI_DMA_INFO* info);
};


class spi_dma {
private:
	static volatile unsigned int* dma_control;
	static volatile unsigned int* spi_control;
	static int instance_count;

	static int is_running;

	std::unique_ptr<gpio> gpio_control;
public:
	spi_dma();
	~spi_dma();
	void start(spi_dma_info_container& info);
};

#endif

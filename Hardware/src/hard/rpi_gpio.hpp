/* 参考PDF[BCM2835-ARM-Peripherals.pdf] */

/* RaspberryPI用GPIO操作プログラム */
/*
対応表

PIN  NAME
0	GPIO 2(SDA)
1	GPIO 3(SCL)
2	GPIO 4(GPCLK0)
3	GPIO 7(CE1)
4	GPIO 8(CE0)
5	GPIO 9(MISO)
6	GPIO 10(MOSI)
7	GPIO 11(SCLK)
8	GPIO 14(TXD)
9	GPIO 15(RXD)
10	GPIO 17
11	GPIO 18(PCM_CLK)
12	GPIO 22
13	GPIO 23
14	GPIO 24
15	GPIO 25
16	GPIO 27(PCM_DOUT)
17	GPIO 28
18	GPIO 29
19	GPIO 30
20	GPIO 31
*/

#ifndef __RPI_GPIO_HPP__
#define __RPI_GPIO_HPP__

/* バスアクセス用物理アドレス(Page.6 - 1.2.3) */
#define PHADDR_OFFSET	0x20000000
/* GPIOコントロールレジスタへのオフセット(Page.90 - 6.1) */
#define GPIO_CONTROL_OFFSET	(PHADDR_OFFSET + 0x200000)

/* GPFSEL0からGPLEVまでのサイズ */
#define GPCONT_SIZE 0x3C

/* 各レジスタへのオフセット */
#define GPFSEL_OFFSET		0x00
#define GPSET_OFFSET		0x1C
#define GPCLR_OFFSET		0x28
#define GPLEV_OFFSET		0x34

/* モード定義 */
#define GPIO_INPUT	0
#define GPIO_OUTPUT	1
#define GPIO_ALT0 4
#define GPIO_ALT1 5
#define GPIO_ALT2 6
#define GPIO_ALT3 7
#define GPIO_ALT4 3
#define GPIO_ALT5 2

#define HIGH	1
#define LOW		0

extern const char* PINtoNAME[];

//GPIOコントロール用クラス
class gpio {
private:
	//GPIOマッピング用ポインタ
	static volatile unsigned int* gpio_control;
	static int instance_count;

public:
	gpio();
	~gpio();

	void mode_write(int pin, int mode);
	void mode_read(int pin, int* mode);
	void data_write(int pin, int data);
	void data_read(int pin, int* data);
};

#endif




#include <stdlib.h>
#include <string>

#include "memory.h"
#include "rpi_gpio.hpp"

//ピン名対応表
const char* PINtoNAME[] = {
	"GPIO 0", "GPIO 1", "GPIO 4", "GPIO 7", "GPIO 8",
	"GPIO 9", "GPIO10", "GPIO11", "GPIO14", "GPIO15",
	"GPIO17", "GPIO18", "GPIO22", "GPIO23", "GPIO24", 
	"GPIO25", "GPIO27", "GPIO28", "GPIO29", "GPIO30",
	"GPIO31"
};

//ビット位置対応表
unsigned char PINtoGPIO[] = {
	 2,  3,  4,  7,  8,
	 9, 10, 11, 14, 15,
	17, 18, 22, 23, 24,
	25, 27, 28, 29, 30,
	31
};

volatile unsigned int* gpio::gpio_control = NULL;
int gpio::instance_count = 0;

gpio::gpio()
{
	//最初のインスタンス化ならば
	if(this->instance_count == 0){
		//GPIOコントロール用レジスタをマッピング
		this->gpio_control = (volatile unsigned int*)map_physic(GPIO_CONTROL_OFFSET, GPCONT_SIZE);

		if(this->gpio_control == NULL) {
			throw std::string("GPIOレジスタのマッピングに失敗しました");
		}
	}
	this->instance_count++;
}

gpio::~gpio()
{
	this->instance_count--;
	//最後のデストラクタならば
	if(this->instance_count == 0){
		unmap_physic((void*)this->gpio_control, GPCONT_SIZE);
	}
}

void gpio::mode_write(int pin, int mode)
{
	if(pin < 0 || pin > sizeof(PINtoGPIO)){
		throw(std::string("mode_write: ピンの値が範囲外です"));
	}

	__asm__ volatile (
		"ldr r0, %0;"
		"add r0, r0, %1;"

		"ldrb r1, [%4, %2];"
		"mov r2, %5;"
		"smull r3, r2, r1, r2;"
		"mov r3, #10;"
		"mul r3, r2, r3;"
		"sub r1, r1, r3;"

		"lsl r2, r2, #2;"
		"add r0, r0, r2;"
		"ldr r2, [r0];"

		"mov r3, #3;"
		"mul r1, r1, r3;"
		"mov r3, #7;"
		"mvn r3, r3, lsl r1;"
		"and r2, r2, r3;"

		"mov r3, %3;"
		"lsl r3, r3, r1;"

		"orr r2, r2, r3;"

		"str r2, [r0];"
		:
		:"m"(this->gpio_control), "n"(GPFSEL_OFFSET), "r"(pin), "r"(mode), "r"(PINtoGPIO), "r"(0x1999999A)
		:"r0", "r1", "r2", "r3"
	);
}

void gpio::mode_read(int pin, int* mode)
{
	if(pin < 0 || pin > sizeof(PINtoGPIO)){
		throw(std::string("mode_read: ピンの値が範囲外です"));
	}

	__asm__ volatile (
		"ldr r0, %1;"
		"add r0, r0, %2;"

		"ldrb r1, [%4, %3];"
		"mov r2, %5;"
		"smull r3, r2, r1, r2;"
		"mov r3, #10;"
		"mul r3, r2, r3;"
		"sub r1, r1, r3;"

		"lsl r2, r2, #2;"
		"add r0, r0, r2;"
		"ldr r0, [r0];"

		"mov r2, #3;"
		"mul r1, r1, r2;"
		"lsr r0, r0, r1;"
		"and r0, r0, #7;"

		"str r0, %0;"
		:"=m"(*mode)
		:"m"(this->gpio_control), "n"(GPFSEL_OFFSET), "r"(pin), "r"(PINtoGPIO), "r"(0x1999999A)
		:"r0","r1", "r2", "r3"
	);
}

void gpio::data_write(int pin, int data)
{
	if(pin < 0 || pin > sizeof(PINtoGPIO)){
		throw(std::string("data_write: ピンの値が範囲外です"));
	}

	__asm__ volatile (
		"tst %4, %4;"

		"ldr r0, %0;"
		"addne r0, r0, %1;"
		"addeq r0, r0, %2;"

		"ldrb r1, [%5, %3];"
		"mov r2, #1;"
		"lsl r2, r2, r1;"

		"str r2, [r0];"
		:
		:"m"(this->gpio_control), "n"(GPSET_OFFSET), "n"(GPCLR_OFFSET), "r"(pin), "r"(data), "r"(PINtoGPIO)
		:"r0","r1", "r2"
	);
}

void gpio::data_read(int pin, int* data)
{
	if(pin < 0 || pin > sizeof(PINtoGPIO)){
		throw(std::string("data_read: ピンの値が範囲外です"));
	}

	__asm__ volatile (
		"ldr r0, %1;"
		"add r0, r0, %2;"

		"ldr r1, [r0];"
		"ldrb r0, [%4, %3];"
		"mov r2, #1;"
		"and r1, r2, r1, lsr r0;"

		"str r1, %0;"
		:"=m"(*data)
		:"m"(this->gpio_control), "n"(GPLEV_OFFSET), "r"(pin), "r"(PINtoGPIO)
		:"r0","r1", "r2"
	);
}

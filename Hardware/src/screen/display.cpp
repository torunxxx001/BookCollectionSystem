#include "display.hpp"

#include <unistd.h>
#include <iostream>

display::display()
{
	this->board_control = std::unique_ptr<board_io>(new board_io);

	this->image = gdImageCreateTrueColor(TFT_WIDTH, TFT_HEIGHT);

	this->image_idx_table.reset(new unsigned int*[TFT_WIDTH * TFT_HEIGHT]);
	//this->image->tpixelsの2次元配列を1次元配列ポインタに
	int idx = 0;
	for(int y = 0; y < TFT_HEIGHT; y++){
		for(int x = TFT_WIDTH - 1; x >= 0; x--){
			this->image_idx_table.get()[idx++] = (unsigned int*)&this->image->tpixels[y][x];
		}
	}

	gdImageFilledRectangle(this->image, 0, 0, TFT_WIDTH, TFT_HEIGHT, gdTrueColor(255, 255, 255));

	this->display_status = DISPLAY_STATUS_OFF;

	this->is_running = 0;
	this->terminate = 0;

	this->setFPS(60.0);
}

display::~display()
{
	this->exit();
	gdImageDestroy(this->image);
}

void display::draw()
{
	int num_pixels = TFT_WIDTH * TFT_HEIGHT * 3;
	unsigned char* pixel_buffer = new unsigned char[num_pixels];

	__asm__ volatile (
		"mov r0, #0;"
		"mov r3, #0;"

		"1:"
		/* idxループ */

		/* ピクセルへのアドレスを生成 */
		"ldr r2, [%1, r0, lsl #2];"
		"ldr r1, [r2];"

		/* 保存 */
		"str r1, [%0, r3];"
		"add r3, #3;"

		"add r0, #1;"
		"teq r0, %2;"
		"bne 1b;"
		/* idxループ終わり */

		:
		:"r"(pixel_buffer), "r"(this->image_idx_table.get()), "r"(TFT_WIDTH * TFT_HEIGHT)
		:"r0", "r1", "r2", "r3"
	);


	//描画
	this->board_control->tft_write_pixels(0x2C, pixel_buffer, num_pixels);

	delete[] pixel_buffer;
}

gdImagePtr display::get_image()
{
	return this->image;
}

void display::setFPS(double fps_val)
{
	this->fps = fps_val;
}

void display::onInit()
{

}

void display::onDeinit()
{

}

void display::onDraw()
{
	this->draw();
}

void display::process()
{
	this->is_running = 1;

	try {
		this->display_on();

		this->onInit();
		while(this->terminate == 0){
			this->onDraw();

			usleep((1 / this->fps) * 1000000);
		}
		this->onDeinit();

		this->display_off();
	} catch(std::string message) {
		this->is_running = 0;

		throw message;
	}

	this->is_running = 0;
}

void* display::thread_proc(void* arg)
{
	//このスレッドでの例外補足
	try {
		((display*)arg)->process();
	} catch(std::string message) {
		std::cerr << "display-error: " << message << std::endl;
	}

	pthread_exit(NULL);
}

int display::get_running_status()
{
	return this->is_running;
}

void display::start()
{
	if(this->is_running){
		this->exit();
	}

	this->terminate = 0;

	pthread_create(&this->thread_handle, NULL, display::thread_proc, (void*)this);

	//スレッドが開始されるまで待つ
	for(int i = 0; i < 1000; i++){
		if(this->is_running) break;
		usleep(100);
	}
}

void display::exit()
{
	if(this->is_running){
		this->terminate = 1;
		pthread_join(this->thread_handle, NULL);
	}
}

void display::display_on()
{
	this->board_control->set_tft_status(DISPLAY_STATUS_ON);

	this->display_status = DISPLAY_STATUS_ON;
}

void display::display_off()
{
	this->board_control->set_tft_status(DISPLAY_STATUS_OFF);

	this->display_status = DISPLAY_STATUS_OFF;
}

int display::get_display_status()
{
	return this->display_status;
}
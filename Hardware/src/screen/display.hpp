#ifndef __DISPLAY_HPP__
#define __DISPLAY_HPP__

#include "board_io.hpp"
#include <gd.h>
#include <pthread.h>
#include <memory>

//継承可能
class display {
private:
	double fps;

	//スレッド関連
	pthread_t thread_handle;
	int is_running;
	int terminate;

	gdImagePtr image; //画面パレット

	//gdImagePtr imageの2次元配列->1次元配列ポインタ用
	std::unique_ptr<unsigned int*> image_idx_table;

	int display_status;

	void draw();
protected:
	std::unique_ptr<board_io> board_control; //基板制御用

	gdImagePtr get_image();
public:
	display();
	virtual ~display();

	void setFPS(double fps_val);

	virtual void onInit();
	virtual void onDeinit();
	virtual void onDraw();

	void process();
	static void* thread_proc(void* arg);

	int get_running_status();
	void start();
	void exit();

	void display_on();
	void display_off();

	int get_display_status();
};

#endif

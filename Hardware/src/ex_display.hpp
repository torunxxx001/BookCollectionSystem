#ifndef __EX_DISPLAY_HPP__
#define __EX_DISPLAY_HPP__

#include "display.hpp"
#include "scr_obj.hpp"

#include <fstream>

//ディスプレイ表示拡張版
class ex_display : public display {
private:
	std::string touch_setting_file_path;

	POINT touch_setting[4];

	screen_container screens;
	std::string old_screen_id;

	int touch_status;

	int sleep_wait_time;
	int captured_time;
public:
	ex_display(std::string screen_list_file, std::string touch_setting_file);
	~ex_display();

	void start_touch_adjust();
	void load_touch_setting();
	int get_touch_xy(POINT* put_pos);

	void onInit();
	void onDraw();
};

#endif

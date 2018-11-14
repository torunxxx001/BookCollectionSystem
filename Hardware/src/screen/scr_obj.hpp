#ifndef __SCR_OBJ_HPP__
#define __SCR_OBJ_HPP__

#include "base.hpp"

#include <vector>
#include <map>
#include <boost/any.hpp>
#include <string>
#include <functional>
#include <memory>
#include <typeinfo>
#include <gd.h>

//再描画必要タイプ
enum {
	DRAW_NO, DRAW_OBJECT, DRAW_CLEAR
};

//クラスプロトタイプ宣言
class screen;

//オブジェクト単体の持つ機能
// タッチ検出

//継承可能
class scr_object {
private:
	std::string object_id;

	POINT position;
	POINT size;

	int visible;
	int enable;

	gdImagePtr obj_image;

	std::function<void (scr_object*)> onPushUpExtra;
	std::function<void (scr_object*)> onPushDownExtra;

	screen* parent_screen;

protected:
	std::string type;

	int need_draw;

	std::map<std::string, boost::any> attributes;

	scr_object(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y);

public:
	typedef std::shared_ptr<scr_object> ScrObjectPtr;

	static ScrObjectPtr create(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y);
	virtual ~scr_object();

	scr_object* set_parent(screen* parent_scr);
	screen* get_parent();

	std::string get_id();
	std::string get_type();

	gdImagePtr get_image();

	int get_size_x();
	int get_size_y();
	int get_pos_x();
	int get_pos_y();
	scr_object* set_pos(int x, int y);
	scr_object* set_size(int x, int y);

	scr_object* clear_need_draw();
	scr_object* set_need_draw(int value);
	int get_need_draw();

	int get_visible();
	int get_enable();
	scr_object* set_visible(int visible);
	scr_object* set_enable(int enable);

	scr_object* set_attr(std::string name, boost::any value);
	boost::any* get_attr(std::string name);
	const std::type_info* get_attr_type(std::string name);

	scr_object* set_pushup_extra(std::function<void (scr_object*)> func);
	scr_object* set_pushdown_extra(std::function<void (scr_object*)> func);

	void call_pushup_extra();
	void call_pushdown_extra();

	void onPushUp();
	void onPushDown();

	virtual void onChangeSetting();

	virtual void onPushUpDraw();
	virtual void onPushDownDraw();
};

class screen {
private:
	std::string screen_id;

	std::map<std::string, scr_object::ScrObjectPtr> scr_object_list;

	//順序関係
	std::vector<scr_object::ScrObjectPtr> obj_order_list;
	std::map<std::string, int> obj_order_index_list;

	scr_object::ScrObjectPtr object_pushup;

	scr_object::ScrObjectPtr dummy_object;

	int color;
	int direction;


	std::function<void (screen*)> onLoadExtra;
	std::function<void (screen*)> onUnloadExtra;
	std::function<void (screen*)> onIntervalExtra;
protected:
	screen(std::string screen_id);

public:
	typedef std::shared_ptr<screen> ScreenPtr;

	static screen::ScreenPtr create(std::string screen_id);
	~screen();

	std::string get_id();

	scr_object::ScrObjectPtr get_object(std::string object_id);

	screen* add(scr_object::ScrObjectPtr object);
	screen* remove(std::string object_id);

	screen* set_color(int color);
	int get_color();

	screen* set_direction(int direction);
	int get_direction();

	void call_load_extra();
	void call_unload_extra();
	void call_interval_extra();

	screen* set_load_extra(std::function<void (screen*)> func);
	screen* set_unload_extra(std::function<void (screen*)> func);
	screen* set_interval_extra(std::function<void (screen*)> func);

	std::vector<scr_object::ScrObjectPtr> get_object_list(std::string obj_type);
	std::vector<scr_object::ScrObjectPtr> get_drawable();
	std::vector<scr_object::ScrObjectPtr> get_need_draw();
	void send_pushup();
	void send_pushdown(int x, int y);
};

class screen_container {
private:
	screen::ScreenPtr current_screen;

	std::map<std::string, screen::ScreenPtr> screen_list;

	screen::ScreenPtr dummy_screen;

	std::function<void ()> onCreateExtra;
	std::function<void ()> onDestroyExtra;

	std::function<void (screen*)> onSelectExtra;
	std::function<void (screen*)> onLoadedExtra;
public:
	screen_container();
	~screen_container();

	void call_create_extra();
	void call_destroy_extra();

	void call_select_extra(screen* scr);
	void call_loaded_extra(screen* scr);

	screen_container* set_create_extra(std::function<void ()> func);
	screen_container* set_destroy_extra(std::function<void ()> func);

	screen_container* set_select_extra(std::function<void (screen*)> func);
	screen_container* set_loaded_extra(std::function<void (screen*)> func);

	screen::ScreenPtr get_screen(std::string screen_id);

	screen_container* add(screen::ScreenPtr scr);
	screen_container* remove(std::string screen_id);

	screen_container* select(std::string screen_id);
	screen::ScreenPtr cur();
};

#endif

#ifndef __FB_SCR_OBJ_HPP__
#define __FB_SCR_OBJ_HPP__

#include "scr_obj.hpp"
#include "framebuffer.hpp"

#include <memory>

class fb_object : public scr_object {
private:
	fb_object(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y);

	int enable;

	//framebufferクラス
	static std::unique_ptr<framebuffer> fb_con;
public:
	typedef std::shared_ptr<fb_object> Fb_objectPtr;


	static Fb_objectPtr create(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y);
	~fb_object();

	void onChangeSetting();
	void onPushUpDraw();
	void onPushDownDraw();
};

#endif

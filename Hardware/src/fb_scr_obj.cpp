#include "fb_scr_obj.hpp"

std::unique_ptr<framebuffer> fb_object::fb_con;

//fb_objectの実装
fb_object::fb_object(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y)
	: scr_object(obj_id, pos_x, pos_y, size_x, size_y)
{
	this->type = "fb_object";

	if( !fb_object::fb_con){
		fb_object::fb_con.reset(new framebuffer(size_x, size_y, R_90));
		fb_object::fb_con->set_image(this->get_image());

		this->enable = 1;
	}else{
		this->enable = 0;
	}

	this->attributes["rotate"] = R_90;

	this->onPushUpDraw();
}

fb_object::Fb_objectPtr fb_object::create(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y)
{
	return fb_object::Fb_objectPtr(new fb_object(obj_id, pos_x, pos_y, size_x, size_y));
}

fb_object::~fb_object()
{

}

void fb_object::onChangeSetting()
{
	if( !this->enable) return;

	boost::any* val_tmp = this->get_attr("rotate");

	if(val_tmp){
		FB_DEGREE tmp_deg = boost::any_cast<FB_DEGREE>(*val_tmp);

		fb_object::fb_con->set_rotate(tmp_deg);
		fb_object::fb_con->update();
	}
}

void fb_object::onPushUpDraw()
{
	if( !this->enable) return;

	fb_object::fb_con->update();

	//描画必要フラグセット
	this->need_draw = DRAW_OBJECT;
}

void fb_object::onPushDownDraw()
{

}


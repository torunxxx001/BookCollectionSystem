#ifndef __EX_SCR_OBJ_HPP__
#define __EX_SCR_OBJ_HPP__

#include "scr_obj.hpp"

class button : public scr_object {
private:
	button(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y);

public:
	typedef std::shared_ptr<button> ButtonPtr;


	static ButtonPtr create(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y);
	~button();

	void onPushUpDraw();
	void onPushDownDraw();
};

//textは継承可能
class text : public scr_object {
protected:
	text(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y);

public:
	typedef std::shared_ptr<text> TextPtr;


	static TextPtr create(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y);
	virtual ~text();

	virtual void onPushUpDraw();
	virtual void onPushDownDraw();
};

//textを継承したpassword
class password : public text {
private:
	password(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y);

public:
	typedef std::shared_ptr<password> PasswordPtr;

	static PasswordPtr create(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y);
	~password();

	void onPushUpDraw();
	void onPushDownDraw();
};

class image : public scr_object {
private:
	gdImagePtr current_image;

	image(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y);

public:
	typedef std::shared_ptr<image> ImagePtr;


	static ImagePtr create(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y);
	~image();

	void onChangeSetting();
	void onPushUpDraw();
	void onPushDownDraw();
};

#endif

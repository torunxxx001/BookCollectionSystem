#include "ex_scr_obj.hpp"

#include "img_lib.hpp"
#include "dstring.hpp"

#include <fstream>
#include <vector>

//buttonの実装
button::button(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y)
	: scr_object(obj_id, pos_x, pos_y, size_x, size_y)
{
	this->type = "button";

	this->attributes["text_color"] = gdTrueColor(0, 0, 0);
	this->attributes["border_color"] = gdTrueColor(0, 0, 0);

	this->attributes["text"] = std::string("");
	this->attributes["font_size"] = 12;

	this->onPushUpDraw();
}

button::ButtonPtr button::create(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y)
{
	return button::ButtonPtr(new button(obj_id, pos_x, pos_y, size_x, size_y));
}

button::~button()
{

}

void button::onPushUpDraw()
{
	int color = boost::any_cast<int>(this->attributes["color"]);
	int border_color = boost::any_cast<int>(this->attributes["border_color"]);
	int text_color = boost::any_cast<int>(this->attributes["text_color"]);
	std::string text = boost::any_cast<std::string>(this->attributes["text"]);

	dstring dstr(boost::any_cast<int>(this->attributes["font_size"]));

	dstr.draw_string(this->get_image(), 1, 1, this->get_size_x() - 2, this->get_size_y() - 2,
		HCENTER, VCENTER, text_color, color, text);

	gdImageRectangle(this->get_image(), 0, 0, this->get_size_x() - 1, this->get_size_y() - 1, border_color);

	//描画必要フラグセット
	this->need_draw = DRAW_OBJECT;
}

void button::onPushDownDraw()
{
	int push_color = boost::any_cast<int>(this->attributes["push_color"]);
	int border_color = boost::any_cast<int>(this->attributes["border_color"]);
	int text_color = boost::any_cast<int>(this->attributes["text_color"]);
	std::string text = boost::any_cast<std::string>(this->attributes["text"]);

	dstring dstr(boost::any_cast<int>(this->attributes["font_size"]));

	dstr.draw_string(this->get_image(), 1, 1, this->get_size_x() - 2, this->get_size_y() - 2,
		HCENTER, VCENTER, text_color, push_color, text);

	gdImageRectangle(this->get_image(), 0, 0, this->get_size_x() - 1, this->get_size_y() - 1, border_color);

	//描画必要フラグセット
	this->need_draw = DRAW_OBJECT;
}


//textの実装
text::text(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y)
	: scr_object(obj_id, pos_x, pos_y, size_x, size_y)
{
	this->type = "text";

	this->attributes["text_color"] = gdTrueColor(0, 0, 0);
	this->attributes["border_color"] = gdTrueColor(0, 0, 0);

	this->attributes["text"] = std::string("");
	this->attributes["font_size"] = 12;

	this->attributes["halign"] = HLEFT;
	this->attributes["valign"] = VTOP;

	this->onPushUpDraw();
}

text::TextPtr text::create(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y)
{
	return text::TextPtr(new text(obj_id, pos_x, pos_y, size_x, size_y));
}

text::~text()
{

}

void text::onPushUpDraw()
{
	STR_ALIGN halign = boost::any_cast<STR_ALIGN>(this->attributes["halign"]);
	STR_ALIGN valign = boost::any_cast<STR_ALIGN>(this->attributes["valign"]);

	int color = boost::any_cast<int>(this->attributes["color"]);
	int border_color = boost::any_cast<int>(this->attributes["border_color"]);
	int text_color = boost::any_cast<int>(this->attributes["text_color"]);
	std::string text = boost::any_cast<std::string>(this->attributes["text"]);

	dstring dstr(boost::any_cast<int>(this->attributes["font_size"]));

	dstr.draw_string(this->get_image(), 1, 1, this->get_size_x() - 2, this->get_size_y() - 2,
		halign, valign, text_color, color, text);

	gdImageRectangle(this->get_image(), 0, 0, this->get_size_x() - 1, this->get_size_y() - 1, border_color);

	//描画必要フラグセット
	this->need_draw = DRAW_OBJECT;
}

void text::onPushDownDraw()
{
	STR_ALIGN halign = boost::any_cast<STR_ALIGN>(this->attributes["halign"]);
	STR_ALIGN valign = boost::any_cast<STR_ALIGN>(this->attributes["valign"]);

	int push_color = boost::any_cast<int>(this->attributes["push_color"]);
	int border_color = boost::any_cast<int>(this->attributes["border_color"]);
	int text_color = boost::any_cast<int>(this->attributes["text_color"]);
	std::string text = boost::any_cast<std::string>(this->attributes["text"]);

	dstring dstr(boost::any_cast<int>(this->attributes["font_size"]));

	dstr.draw_string(this->get_image(), 1, 1, this->get_size_x() - 2, this->get_size_y() - 2,
		halign, valign, text_color, push_color, text);

	gdImageRectangle(this->get_image(), 0, 0, this->get_size_x() - 1, this->get_size_y() - 1, border_color);

	//描画必要フラグセット
	this->need_draw = DRAW_OBJECT;
}

//passwordの実装
password::password(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y)
	: text(obj_id, pos_x, pos_y, size_x, size_y)
{
	this->type = "password";

	this->onPushUpDraw();
}

password::PasswordPtr password::create(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y)
{
	return password::PasswordPtr(new password(obj_id, pos_x, pos_y, size_x, size_y));
}

password::~password()
{

}

void password::onPushUpDraw()
{
	STR_ALIGN halign = boost::any_cast<STR_ALIGN>(this->attributes["halign"]);
	STR_ALIGN valign = boost::any_cast<STR_ALIGN>(this->attributes["valign"]);

	int color = boost::any_cast<int>(this->attributes["color"]);
	int border_color = boost::any_cast<int>(this->attributes["border_color"]);
	int text_color = boost::any_cast<int>(this->attributes["text_color"]);
	std::string text = boost::any_cast<std::string>(this->attributes["text"]);

	dstring dstr(boost::any_cast<int>(this->attributes["font_size"]));

	//パスワード文字に置き換える
	std::string pass_str = "";
	pass_str.resize(text.length(), '*');

	dstr.draw_string(this->get_image(), 1, 1, this->get_size_x() - 2, this->get_size_y() - 2,
		halign, valign, text_color, color, pass_str);

	gdImageRectangle(this->get_image(), 0, 0, this->get_size_x() - 1, this->get_size_y() - 1, border_color);

	//描画必要フラグセット
	this->need_draw = DRAW_OBJECT;
}

void password::onPushDownDraw()
{
	STR_ALIGN halign = boost::any_cast<STR_ALIGN>(this->attributes["halign"]);
	STR_ALIGN valign = boost::any_cast<STR_ALIGN>(this->attributes["valign"]);

	int push_color = boost::any_cast<int>(this->attributes["push_color"]);
	int border_color = boost::any_cast<int>(this->attributes["border_color"]);
	int text_color = boost::any_cast<int>(this->attributes["text_color"]);
	std::string text = boost::any_cast<std::string>(this->attributes["text"]);

	dstring dstr(boost::any_cast<int>(this->attributes["font_size"]));

	//パスワード文字に置き換える
	std::string pass_str = "";
	pass_str.resize(text.length(), '*');

	dstr.draw_string(this->get_image(), 1, 1, this->get_size_x() - 2, this->get_size_y() - 2,
		halign, valign, text_color, push_color, pass_str);

	gdImageRectangle(this->get_image(), 0, 0, this->get_size_x() - 1, this->get_size_y() - 1, border_color);

	//描画必要フラグセット
	this->need_draw = DRAW_OBJECT;
}


//imageの実装
image::image(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y)
	: scr_object(obj_id, pos_x, pos_y, size_x, size_y)
{
	this->type = "image";

	this->current_image = NULL;

	this->attributes["path"] = std::string("");
	this->attributes["data"] = std::vector<unsigned char>();
	this->attributes["angle"] = 0.0;

	this->onPushUpDraw();
}

image::ImagePtr image::create(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y)
{
	return image::ImagePtr(new image(obj_id, pos_x, pos_y, size_x, size_y));
}

image::~image()
{
	if(this->current_image != NULL){
		gdImageDestroy(this->current_image);
	}
}

void image::onChangeSetting()
{
	//設定が変更されるたびに呼び出される

	std::vector<unsigned char> img_data = boost::any_cast<std::vector<unsigned char> >(this->attributes["data"]);
	std::string path = boost::any_cast<std::string>(this->attributes["path"]);

	//dataが未設定ならパスからの読取を試みる
	if(img_data.size() == 0 && path != ""){
		std::ifstream ifs(path, std::ios::in | std::ios::binary);

		if(ifs){
			unsigned char buffer[1024];

			while(!ifs.eof()){
				ifs.read((char*)buffer, sizeof(buffer));

				std::copy(buffer, buffer + ifs.gcount(), std::inserter(img_data, img_data.end()));
			}

			ifs.close();
		}
	}

	//ポインタのデータからGD画像を作成
	if(img_data.size() > 0){
		gdImagePtr load_image = img_lib::ImageCreateFromPtr(img_data);

		//ロードに成功したら画像ポインタ格納
		if(load_image != NULL){
			if(this->current_image != NULL){
				gdImageDestroy(this->current_image);
			}

			this->current_image = load_image;
		}
	}

	//attributesのdataとpathをクリア
	this->attributes["data"] = std::vector<unsigned char>();
	this->attributes["path"] = std::string("");


	//ロード済み画像があれば
	if(this->current_image != NULL){

		//回転の設定によって分岐
		double angle = boost::any_cast<double>(this->attributes["angle"]);

		if(angle == 0.0){
			//リサイズコピー
			img_lib::ImageCopyResized(this->get_image(), this->current_image, 0, 0,
				0, 0, this->get_size_x(), this->get_size_y(), gdImageSX(this->current_image), gdImageSY(this->current_image));
		}else{
			int dest_size_x = 0;
			int dest_size_y = 0;

			//回転後のサイズを計算
			img_lib::ImageCopyRotated(NULL, NULL, 0, 0,
				0, 0, &dest_size_x, &dest_size_y, gdImageSX(this->current_image), gdImageSY(this->current_image), angle);

			//一時画像作成
			gdImagePtr tmp_image = gdImageCreateTrueColor(dest_size_x, dest_size_y);

			//一時画像を指定色で塗りつぶし
			int color = boost::any_cast<int>(this->attributes["color"]);
			gdImageFilledRectangle(tmp_image, 0, 0, dest_size_x, dest_size_y, color);

			//一時画像に回転させた画像を格納
			img_lib::ImageCopyRotated(tmp_image, this->current_image, 0, 0,
				0, 0, &dest_size_x, &dest_size_y, gdImageSX(this->current_image), gdImageSY(this->current_image), angle);

			//リサイズコピー
			img_lib::ImageCopyResized(this->get_image(), tmp_image, 0, 0,
				0, 0, this->get_size_x(), this->get_size_y(), dest_size_x, dest_size_y);

			gdImageDestroy(tmp_image);
		}
	}


	//オブジェクト画像更新
	this->onPushUpDraw();
}

void image::onPushUpDraw()
{
	int color = boost::any_cast<int>(this->attributes["color"]);

	gdImageRectangle(this->get_image(), 0, 0, this->get_size_x() - 1, this->get_size_y() - 1, color);

	//描画必要フラグセット
	this->need_draw = DRAW_OBJECT;
}

void image::onPushDownDraw()
{
	int push_color = boost::any_cast<int>(this->attributes["push_color"]);

	gdImageRectangle(this->get_image(), 0, 0, this->get_size_x() - 1, this->get_size_y() - 1, push_color);

	//描画必要フラグセット
	this->need_draw = DRAW_OBJECT;
}

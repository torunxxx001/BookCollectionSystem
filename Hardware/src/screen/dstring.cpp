#include "dstring.hpp"
#include "base.hpp"

#include <string.h>

#include <iostream>

dstring::dstring(int font_size)
{
	this->set_font_size(font_size);
}

void dstring::calc_size(std::string in_char, POINT* ret_pos, POINT* ret_size)
{
	int rect[8];
	gdImageStringFT(NULL, rect, 0, FONT_PATH, this->font_size, 0.0, 0, 0, (char*)in_char.c_str());

	ret_pos->x = 0;
	ret_pos->y = this->font_y_offset;

	ret_size->x = ABS(rect[2] - rect[0]);
	ret_size->y = this->font_height;
}

void dstring::set_font_size(int font_size)
{
	this->font_size = font_size;

	//高さ再計算
	int rect[8];
	gdImageStringFT(NULL, rect, 0, FONT_PATH, this->font_size, 0.0, 0, 0, (char*)"あ");
	this->font_height = ABS(rect[1] - rect[7]);
	this->font_y_offset = ABS(rect[7]);
}


//文字描画域のエリア計算メソッド
std::vector<STR_SIZE> dstring::calc_string_area(std::string in_str, int width, POINT* ret_max_size)
{
	std::vector<STR_SIZE> string_size_list;

	ret_max_size->x = 0;
	ret_max_size->y = 0;

	std::string str_tmp, old_str_tmp;
	for(int index = 0; index < in_str.length(); index++){
		//文字の長さ計算
		char str_char = in_str[index];
		int char_size = 1;
		//UTF-8マルチバイト文字なら
		if(str_char >= 0x80){
			//構成バイト数抜き出し
			int utf8_size = 2;
			if(((str_char >> 5) & 1) == 0) utf8_size = 2;
			else if(((str_char >> 4) & 1) == 0) utf8_size = 3;
			else if(((str_char >> 3) & 1) == 0) utf8_size = 4;

			char_size = utf8_size;
		}

		//追加前の文字列を保存
		old_str_tmp = str_tmp;

		//文字列追加
		str_tmp += in_str.substr(index, char_size);

		//文字サイズ計算
		POINT pos_tmp;
		POINT size_tmp;
		this->calc_size(str_tmp, &pos_tmp, &size_tmp);

		//幅オーバーか改行か文字列の終端なら実行
		if(size_tmp.x > width || str_char == '\n' || index + char_size >= in_str.length()){
			//幅オーバーで入ってきたならば修正し再計算
			if(size_tmp.x > width){
				//文字追加前の文字列が空だったならば終了
				if(old_str_tmp.length() == 0){
					break;
				}

				str_tmp = old_str_tmp;
				this->calc_size(str_tmp, &pos_tmp, &size_tmp);

				//文字オーバーの場合はループを１つ前からやり直し
				index -= char_size;
			}

			STR_SIZE str_size_tmp;

			str_size_tmp.text = str_tmp;
			str_size_tmp.width_pixel = size_tmp.x;
			str_size_tmp.height_pixel = size_tmp.y;
			str_size_tmp.x_offset = pos_tmp.x;
			str_size_tmp.y_offset = pos_tmp.y;

			int tmp_sum_y = size_tmp.y;
			if(string_size_list.size() > 0){
				int b_sum_y = 0;
				for(int li = 0; li < string_size_list.size(); li++){
					b_sum_y += string_size_list[li].height_pixel;
				}
				str_size_tmp.y_offset += b_sum_y;

				tmp_sum_y += b_sum_y;
			}
			if(tmp_sum_y > ret_max_size->y) ret_max_size->y = tmp_sum_y;
			if(size_tmp.x > ret_max_size->x) ret_max_size->x = size_tmp.x;

			string_size_list.push_back(str_size_tmp);

			str_tmp = "";
		}

		index += char_size - 1;
	}

	return string_size_list;
}

int dstring::draw_string(gdImagePtr image, int x, int y, int width, int height, STR_ALIGN halign, STR_ALIGN valign, int fontcolor, int backcolor, std::string in_str)
{
	//文字の後ろを塗りつぶす
	if(backcolor != (int)(-1)){
		POINT rect_p[2];

		rect_p[0].x = x;
		rect_p[0].y = y;
		rect_p[1].x = x + width - 1;
		rect_p[1].y = y + height - 1;

		gdImageFilledRectangle(image, rect_p[0].x, rect_p[0].y, rect_p[1].x, rect_p[1].y, backcolor);
	}

	std::vector<STR_SIZE> size_list;
	POINT cSize;

	size_list = this->calc_string_area(in_str, width, &cSize);

	int start_y = y;

	//valignに応じて揃える
	switch(valign){
		case VCENTER: start_y += (height - cSize.y) / 2; break;
		case VBOTTOM: start_y += (height - cSize.y); break;
	}

	int sum_height = 0;
	int old_ptr = 0;
	for(int index = 0; index < size_list.size(); index++){
		STR_SIZE *size_ptr = &size_list[index];

		sum_height += size_ptr->height_pixel;
		//高さがオーバーするなら終了
		if(sum_height > height){
			break;
		}

		POINT rect_p;
		rect_p.x = x + size_ptr->x_offset;
		rect_p.y = start_y + size_ptr->y_offset;

		//halignに応じて揃える
		switch(halign){
			case HCENTER: rect_p.x += (width - size_ptr->width_pixel) / 2; break;
			case HRIGHT: rect_p.x += (width - size_ptr->width_pixel); break;
		}

		gdImageStringFT(image, NULL, fontcolor, FONT_PATH, this->font_size, 0, rect_p.x, rect_p.y, (char *)size_ptr->text.c_str());
	}

	return y + height;
}

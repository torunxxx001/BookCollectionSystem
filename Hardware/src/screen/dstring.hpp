#ifndef __DSTRING_HPP__
#define __DSTRING_HPP__

#include "base.hpp"

#include <vector>
#include <string>
#include <gd.h>

#define FONT_PATH (char*)"/usr/share/fonts/truetype/IPAFont/ipagp.ttf"

typedef enum {
	HLEFT, HCENTER, HRIGHT,
	VTOP, VCENTER, VBOTTOM
} STR_ALIGN;

typedef struct {
	std::string text;
	int width_pixel;
	int height_pixel;

	int x_offset;
	int y_offset;
} STR_SIZE;

class dstring {
private:
	int font_size;
	int font_height;
	int font_y_offset;

	void calc_size(std::string in_char, POINT* ret_pos, POINT* ret_size);
public:
	dstring(int font_size);

	void set_font_size(int font_size);

	std::vector<STR_SIZE> calc_string_area(std::string in_str, int width, POINT* ret_max_size);

	int draw_string(gdImagePtr image, int x, int y, int width, int height, STR_ALIGN halign, STR_ALIGN valign, int fontcolor, int backcolor, std::string in_str);
};

#endif

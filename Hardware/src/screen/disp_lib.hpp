#ifndef __DISP_LIB_HPP__
#define __DISP_LIB_HPP__

#include "base.hpp"

#include <gd.h>

enum{
	DIRECTION_NORMAL,
	DIRECTION_UP,
	DIRECTION_RIGHT,
	DIRECTION_LEFT
};

class disp_lib {
public:
	static void calcCoord(int direction, POINT* points, int point_len);
	static void calcCoordReverse(int direction, POINT* points, int point_len);
	static void swapCoord(POINT *pos1, POINT *pos2);
	static void copyImage(gdImagePtr dest, gdImagePtr src, int dest_x, int dest_y, int src_x, int src_y, int width, int height, int direction);
};

#endif

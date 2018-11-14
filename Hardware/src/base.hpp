#ifndef __BASE_HPP__
#define __BASE_HPP__

typedef struct {
	int x;
	int y;
} POINT;

typedef struct {
	int left;
	int top;
	int right;
	int bottom;
} RECT;

#define ABS(val) ((val) > 0 ? (val) : ((val) * (-1)))
#define PI 3.141592653

#define ROUND_UP(val) ((int)(val >= 0 ? (val + 0.999999999) : (val - 0.999999999)))


//TFTÇÃÉTÉCÉY
#define TFT_WIDTH 240
#define TFT_HEIGHT 320

#endif

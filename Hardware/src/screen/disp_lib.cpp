#include "disp_lib.hpp"
#include "display.hpp"
#include "img_lib.hpp"

void disp_lib::calcCoord(int direction, POINT* points, int point_len)
{
	switch(direction){
		case DIRECTION_UP:
			for(int index = 0; index < point_len; index++){
				int x = points[index].x;
				int y = points[index].y;

				points[index].x = TFT_WIDTH - x - 1;
				points[index].y = TFT_HEIGHT - y - 1;
			}
		break;
		case DIRECTION_RIGHT:
			for(int index = 0; index < point_len; index++){
				int x = points[index].x;
				int y = points[index].y;

				points[index].x = y;
				points[index].y = TFT_HEIGHT - x - 1;
			}
		break;
		case DIRECTION_LEFT:
			for(int index = 0; index < point_len; index++){
				int x = points[index].x;
				int y = points[index].y;

				points[index].x = TFT_WIDTH - y - 1;
				points[index].y = x;
			}
		break;
	}

	for(int index = 0; index < point_len; index++){
		int *x = &points[index].x;
		int *y = &points[index].y;

		if(*x < 0) *x = 0;
		if(*y < 0) *y = 0;

		switch(direction){
			case DIRECTION_RIGHT:
			case DIRECTION_LEFT:
				if(*x >= TFT_HEIGHT) *x = TFT_HEIGHT - 1;
				if(*y >= TFT_WIDTH) *y = TFT_WIDTH - 1;
			break;
			case DIRECTION_UP:
			default:
				if(*x >= TFT_WIDTH) *x = TFT_WIDTH - 1;
				if(*y >= TFT_HEIGHT) *y = TFT_HEIGHT - 1;
			break;
		}
	}
}

void disp_lib::calcCoordReverse(int direction, POINT* points, int point_len)
{
	switch(direction){
		case DIRECTION_UP:
			for(int index = 0; index < point_len; index++){
				int x = points[index].x;
				int y = points[index].y;

				points[index].x = TFT_WIDTH - x - 1;
				points[index].y = TFT_HEIGHT - y - 1;
			}
		break;
		case DIRECTION_RIGHT:
			for(int index = 0; index < point_len; index++){
				int x = points[index].x;
				int y = points[index].y;

				points[index].x = TFT_HEIGHT - y - 1;
				points[index].y = x;
			}
		break;
		case DIRECTION_LEFT:
			for(int index = 0; index < point_len; index++){
				int x = points[index].x;
				int y = points[index].y;

				points[index].x = y;
				points[index].y = TFT_WIDTH - x - 1;
			}
		break;
	}

	for(int index = 0; index < point_len; index++){
		int *x = &points[index].x;
		int *y = &points[index].y;

		if(*x < 0) *x = 0;
		if(*y < 0) *y = 0;

		switch(direction){
			case DIRECTION_RIGHT:
			case DIRECTION_LEFT:
				if(*x >= TFT_HEIGHT) *x = TFT_HEIGHT - 1;
				if(*y >= TFT_WIDTH) *y = TFT_WIDTH - 1;
			break;
			case DIRECTION_UP:
			default:
				if(*x >= TFT_WIDTH) *x = TFT_WIDTH - 1;
				if(*y >= TFT_HEIGHT) *y = TFT_HEIGHT - 1;
			break;
		}
	}
}

void disp_lib::swapCoord(POINT *pos1, POINT *pos2)
{
	int tmp;

	if(pos1->x > pos2->x){
		tmp = pos1->x;
		pos1->x = pos2->x;
		pos2->x = tmp;
	}
	if(pos1->y > pos2->y){
		tmp = pos1->y;
		pos1->y = pos2->y;
		pos2->y = tmp;
	}
}

void disp_lib::copyImage(gdImagePtr dest, gdImagePtr src, int dest_x, int dest_y, int src_x, int src_y, int width, int height, int direction)
{
	int dest_w_tmp = 0;
	int dest_h_tmp = 0;

	switch(direction){
		case DIRECTION_UP:
			img_lib::ImageCopyRotated(NULL, NULL, TFT_WIDTH - width - dest_x, TFT_HEIGHT - height - dest_y, src_x, src_y, &dest_w_tmp, &dest_h_tmp, width, height, 180);
			img_lib::ImageCopyRotated(dest, src, TFT_WIDTH - width - dest_x, TFT_HEIGHT - height - dest_y, src_x, src_y, &dest_w_tmp, &dest_h_tmp, width, height, 180);
		break;
		case DIRECTION_RIGHT:
			img_lib::ImageCopyRotated(NULL, NULL, dest_y, TFT_HEIGHT - width - dest_x, src_x, src_y, &dest_w_tmp, &dest_h_tmp, width, height, 90);
			img_lib::ImageCopyRotated(dest, src, dest_y, TFT_HEIGHT - width - dest_x, src_x, src_y, &dest_w_tmp, &dest_h_tmp, width, height, 90);
		break;
		case DIRECTION_LEFT:
			img_lib::ImageCopyRotated(NULL, NULL, TFT_WIDTH - height - dest_y, dest_x, src_x, src_y, &dest_w_tmp, &dest_h_tmp, width, height, 270);
			img_lib::ImageCopyRotated(dest, src, TFT_WIDTH - height - dest_y, dest_x, src_x, src_y, &dest_w_tmp, &dest_h_tmp, width, height, 270);
		break;
		default:
			gdImageCopy(dest, src, dest_x, dest_y, src_x, src_y, width, height);
		break;
	}
}

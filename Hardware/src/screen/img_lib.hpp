#ifndef __IMG_LIB_HPP__
#define __IMG_LIB_HPP__

#include <vector>
#include <gd.h>

class img_lib {
public:
	static void ImageCopyRotated(gdImagePtr dest, gdImagePtr src, int dest_x, int dest_y,
								int src_x, int src_y, int *dest_w, int *dest_h, int src_w, int src_h, int angle);
	static void ImageCopyResized(gdImagePtr dest, gdImagePtr src, int dest_x, int dest_y,
								int src_x, int src_y, int dest_w, int dest_h, int src_w, int src_h);

	static gdImagePtr ImageCreateFromPtr(std::vector<unsigned char>& img_data);
};

#endif

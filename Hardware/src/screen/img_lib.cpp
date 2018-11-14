#include "img_lib.hpp"

#include <stdlib.h>
#include <math.h>

#include <map>
#include <vector>
#include <string>

void img_lib::ImageCopyRotated(gdImagePtr dest, gdImagePtr src, int dest_x, int dest_y,
								int src_x, int src_y, int *dest_w, int *dest_h, int src_w, int src_h, int angle)
{
	int int_asin = sin(angle * 0.017453292519943295) * 4096;
	int int_acos = cos(angle * 0.017453292519943295) * 4096;

	int dest_w_tmp = abs((src_w * int_acos + src_h * int_asin) >> 12);
	int dest_h_tmp = abs((src_w * int_asin + src_h * int_acos) >> 12);

	if(*dest_w == 0 || *dest_h == 0){
		*dest_w = dest_w_tmp;
		*dest_h = dest_h_tmp;

		return;
	}

	int src_center_x = src_w / 2;
	int src_center_y = src_h / 2;
	int dest_center_x = dest_w_tmp / 2;
	int dest_center_y = dest_h_tmp / 2;

	int start_x = int_acos * (-dest_center_x) - int_asin * (-dest_center_y) + (src_center_x * 4096);
	int start_y = int_asin * (-dest_center_x) + int_acos * (-dest_center_y) + (src_center_y * 4096);

	if(start_x >> 12 == src_w){
		start_x -= 4096;
	}
	if(start_y >> 12 == src_h){
		start_y -= 4096;
	}

	int x1_mult = 0;
	int y1_mult = 0;

	double LUT[1024 + 1];
	for(int i = 0; i <= 1024; i++){
		LUT[i] = i/1024.0;
	}

	for(int y2 = 0; y2 < dest_h_tmp; y2++){
		x1_mult = start_x - y2 * int_asin;
		y1_mult = start_y + y2 * int_acos;

		for(int x2 = 0; x2 < dest_w_tmp; x2++){
			int x1 = x1_mult >> 12;
			int y1 = y1_mult >> 12;

			if(x2 < *dest_w && y2 < *dest_h){
				if(x1 >= 0 && x1 < src_w && y1 >= 0 && y1 < src_h){
					if(x1 < src_w - 1 && y1 < src_h - 1){
						x1 += src_x;
						y1 += src_y;

						int pix0 = gdImageGetPixel(src, x1, y1);
						int pix1 = gdImageGetPixel(src, x1 + 1, y1);
						int pix2 = gdImageGetPixel(src, x1, y1 + 1);
						int pix3 = gdImageGetPixel(src, x1 + 1, y1 + 1);

						int dx = (x1_mult & 0xFFF) >> 2;
						int dy = (y1_mult & 0xFFF) >> 2;

						double ratio_y = LUT[dy];
						double ratio_ry = LUT[1024 - dy];
						double ratio_x = LUT[dx];
						double ratio_rx = LUT[1024 - dx];

						int red = ratio_ry * (ratio_rx * gdImageRed(src, pix0) + ratio_x * gdImageRed(src, pix1))
									+ ratio_y * (ratio_rx * gdImageRed(src, pix2) + ratio_x * gdImageRed(src, pix3));

						int green = ratio_ry * (ratio_rx * gdImageGreen(src, pix0) + ratio_x * gdImageGreen(src, pix1))
									+ ratio_y * (ratio_rx * gdImageGreen(src, pix2) + ratio_x * gdImageGreen(src, pix3));

						int blue = ratio_ry * (ratio_rx * gdImageBlue(src, pix0) + ratio_x * gdImageBlue(src, pix1))
									+ ratio_y * (ratio_rx * gdImageBlue(src, pix2) + ratio_x * gdImageBlue(src, pix3));

						int alpha = ratio_ry * (ratio_rx * gdImageAlpha(src, pix0) + ratio_x * gdImageAlpha(src, pix1))
									+ ratio_y * (ratio_rx * gdImageAlpha(src, pix2) + ratio_x * gdImageAlpha(src, pix3));


						gdImageSetPixel(dest, x2 + dest_x, y2 + dest_y, gdImageColorAllocateAlpha(dest, red, green, blue, alpha));
					}else{
						int pix = gdImageGetPixel(src, x1 + src_x, y1 + src_y);
						gdImageSetPixel(dest, x2 + dest_x, y2 + dest_y, pix);
					}
				}else{
					gdImageSetPixel(dest, x2 + dest_x, y2 + dest_y, gdImageColorAllocateAlpha(dest, 0, 0, 0, 0xFF));
				}
			}

			x1_mult += int_acos;
			y1_mult += int_asin;
		}
	}
}

void img_lib::ImageCopyResized(gdImagePtr dest, gdImagePtr src, int dest_x, int dest_y,
								int src_x, int src_y, int dest_w, int dest_h, int src_w, int src_h)
{
	int int_ratio_x = ((double)src_w / dest_w) * 4096;
	int int_ratio_y = ((double)src_h / dest_h) * 4096;

	double LUT[1024 + 1];
	for(int i = 0; i <= 1024; i++){
		LUT[i] = i/1024.0;
	}

	for(int y2 = 0; y2 < dest_h; y2++){
		for(int x2 = 0; x2 < dest_w; x2++){
			int x1 = x2 * int_ratio_x;
			int y1 = y2 * int_ratio_y;

			x1 >>= 12;
			y1 >>= 12;
			if(x1 < src_w && y1 < src_h){
				if(x1 < src_w - 1 && y1 < src_h - 1){
					x1 += src_x;
					y1 += src_y;

					int pix0 = gdImageGetPixel(src, x1, y1);
					int pix1 = gdImageGetPixel(src, x1+1, y1);
					int pix2 = gdImageGetPixel(src, x1, y1+1);
					int pix3 = gdImageGetPixel(src, x1+1, y1+1);

					int dx = (x1 & 0xFFF) >> 2;
					int dy = (y1 & 0xFFF) >> 2;

					double ratio_y = LUT[dy];
					double ratio_ry = LUT[1024 - dy];
					double ratio_x = LUT[dx];
					double ratio_rx = LUT[1024 - dx];

					int red = ratio_ry * (ratio_rx * gdImageRed(src, pix0) + ratio_x * gdImageRed(src, pix1))
								+ ratio_y * (ratio_rx * gdImageRed(src, pix2) + ratio_x * gdImageRed(src, pix3));

					int green = ratio_ry * (ratio_rx * gdImageGreen(src, pix0) + ratio_x * gdImageGreen(src, pix1))
								+ ratio_y * (ratio_rx * gdImageGreen(src, pix2) + ratio_x * gdImageGreen(src, pix3));

					int blue = ratio_ry * (ratio_rx * gdImageBlue(src, pix0) + ratio_x * gdImageBlue(src, pix1))
								+ ratio_y * (ratio_rx * gdImageBlue(src, pix2) + ratio_x * gdImageBlue(src, pix3));

					int alpha = ratio_ry * (ratio_rx * gdImageAlpha(src, pix0) + ratio_x * gdImageAlpha(src, pix1))
								+ ratio_y * (ratio_rx * gdImageAlpha(src, pix2) + ratio_x * gdImageAlpha(src, pix3));

					gdImageSetPixel(dest, x2 + dest_x, y2 + dest_y, gdImageColorAllocateAlpha(dest, red, green, blue, alpha));
				}else{
					int pix = gdImageGetPixel(src, x1 + src_x, y1 + src_y);

					gdImageSetPixel(dest, x2 + dest_x, y2 + dest_y, pix);
				}
			}
		}
	}
}

gdImagePtr img_lib::ImageCreateFromPtr(std::vector<unsigned char>& img_data)
{
	std::map<std::string, std::vector<unsigned char> > img_prefix_list;
	std::map<std::string, std::vector<unsigned char> >::iterator pref_idx;

	img_prefix_list["PNG"] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
	img_prefix_list["JPEG"] = {0xFF, 0xD8};
	img_prefix_list["GIF"] = {0x47, 0x49, 0x46};


	//先頭のバイトと一致するかチェック
	std::string img_type;
	for(pref_idx = img_prefix_list.begin(); pref_idx != img_prefix_list.end(); pref_idx++){
		std::string type_tmp = (*pref_idx).first;
		std::vector<unsigned char>& pref_tmp = (*pref_idx).second;

		if(pref_tmp.size() <= img_data.size()){
			std::vector<unsigned char> in_img_prefix;
			std::copy(img_data.begin(), img_data.begin() + pref_tmp.size(),
								std::inserter(in_img_prefix, in_img_prefix.begin()));

			if(in_img_prefix == pref_tmp){
				img_type = type_tmp;
				break;
			}
		}
	}

	gdImagePtr image;

	//画像種類判断
	if(img_type == "PNG"){
		image = gdImageCreateFromPngPtr(img_data.size(), &(*img_data.begin()));
	}else if(img_type == "JPEG"){
		image = gdImageCreateFromJpegPtr(img_data.size(), &(*img_data.begin()));
	}else if(img_type == "GIF"){
		image = gdImageCreateFromGifPtr(img_data.size(), &(*img_data.begin()));
	}else{
		image = NULL;
	}

	return image;
}
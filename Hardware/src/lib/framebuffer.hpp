#ifndef __FRAMEBUFFER_HPP__
#define __FRAMEBUFFER_HPP__

#define FB_DEVICE "/dev/fb0"

#include "base.hpp"

#include <memory>
#include <gd.h>
#include <linux/fb.h>

enum FB_DEGREE {
	R_0, R_90, R_180, R_270
};

class framebuffer {
private:
	static int instance_count;
	static int fb_fd;

	gdImagePtr fb_image;

	std::unique_ptr<unsigned char*> fb_idx_table;
	std::unique_ptr<unsigned int*> image_idx_table;

	int fb_mem_size;
	unsigned char* fb_memory;

	struct fb_var_screeninfo now_fbinfo;

	FB_DEGREE rotate;

	POINT size;

	void init();
	void deinit();

	void make_imgidx_table();
public:
	framebuffer(int width, int height, FB_DEGREE fb_rotate);
	~framebuffer();

	void update();
	void set_image(gdImagePtr image);
	void set_rotate(FB_DEGREE degree);
};

#endif

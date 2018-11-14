#include "framebuffer.hpp"

#include <string>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

int framebuffer::instance_count = 0;
int framebuffer::fb_fd = 0;

framebuffer::framebuffer(int width, int height, FB_DEGREE fb_rotate)
{
	if(framebuffer::instance_count == 0){
		framebuffer::fb_fd = open(FB_DEVICE, O_RDWR);

		if(framebuffer::fb_fd < 0){
			throw std::string("fb device open error");
		}
	}
	framebuffer::instance_count++;

	this->fb_image = NULL;
	memset(&this->now_fbinfo, 0, sizeof(fb_var_screeninfo));
	this->fb_mem_size = 0;
	this->fb_memory = NULL;

	this->size.x = width;
	this->size.y = height;

	this->set_rotate(fb_rotate);

	this->init();
}

framebuffer::~framebuffer()
{
	this->deinit();

	framebuffer::instance_count--;
	if(framebuffer::instance_count == 0){
		close(framebuffer::fb_fd);
	}
}
#include <iostream>
void framebuffer::init()
{
	struct fb_var_screeninfo fbinfo_tmp;
	ioctl(framebuffer::fb_fd, FBIOGET_VSCREENINFO, &fbinfo_tmp);

	int pix_size = (fbinfo_tmp.bits_per_pixel / 8);

	//実際のFBのサイズを計算
	struct fb_fix_screeninfo fbfix_info;
	ioctl(framebuffer::fb_fd, FBIOGET_FSCREENINFO, &fbfix_info);

	int width_tmp = fbfix_info.smem_len / pix_size / fbinfo_tmp.yres;
	int height_tmp = fbinfo_tmp.yres;


	if(fbinfo_tmp.bits_per_pixel != 24
		|| fbinfo_tmp.xres != width_tmp || fbinfo_tmp.yres != height_tmp)
	{
std::cout << "change:" << width_tmp << ", " << height_tmp << std::endl;
		//一応24ビットセット
		fbinfo_tmp.bits_per_pixel = 24;

		//FBサイズを再補正
		fbinfo_tmp.xres = fbinfo_tmp.xres_virtual = width_tmp;
		fbinfo_tmp.yres = fbinfo_tmp.yres_virtual = height_tmp;

		ioctl(framebuffer::fb_fd, FBIOPUT_VSCREENINFO, &fbinfo_tmp);

		this->size.x = width_tmp;
		this->size.y = height_tmp;
	}

	//設定が変わっていれば更新
	if(this->now_fbinfo.xres != fbinfo_tmp.xres || this->now_fbinfo.yres != fbinfo_tmp.yres
		|| this->now_fbinfo.xres_virtual != fbinfo_tmp.xres_virtual || this->now_fbinfo.yres_virtual != fbinfo_tmp.yres_virtual)
	{
		//先に解放
		this->deinit();

		memcpy(&this->now_fbinfo, &fbinfo_tmp, sizeof(fb_var_screeninfo));


		this->fb_mem_size = this->now_fbinfo.xres * this->now_fbinfo.yres * pix_size;
		this->fb_memory = (unsigned char*)mmap(NULL, this->fb_mem_size, PROT_READ, MAP_PRIVATE, framebuffer::fb_fd, 0);
		if(this->fb_memory == MAP_FAILED){
			throw std::string("fb_mem map failed");
		}

		this->fb_idx_table.reset(new unsigned char*[this->now_fbinfo.xres * this->now_fbinfo.yres]);
		int idx = 0;
		for(int y = 0; y < this->now_fbinfo.yres; y++){
			for(int x = 0; x < this->now_fbinfo.xres; x++){
				this->fb_idx_table.get()[idx++] = &this->fb_memory[(y * this->now_fbinfo.xres + x) * pix_size];
			}
		}

		this->make_imgidx_table();
	}
}

void framebuffer::deinit()
{
	if(this->fb_memory != NULL){
		munmap(this->fb_memory, this->fb_mem_size);
		this->fb_memory = NULL;
		this->fb_mem_size = 0;
	}
}

void framebuffer::make_imgidx_table()
{
	if(this->fb_image == NULL) return;

	this->image_idx_table.reset(new unsigned int*[this->now_fbinfo.xres * this->now_fbinfo.yres]);

	int idx = 0;
	switch(this->rotate){
		case R_90:
			for(int x = this->now_fbinfo.yres - 1; x >= 0; x--){
				for(int y = 0; y < this->now_fbinfo.xres; y++){
					if(x < gdImageSX(this->fb_image) && y < gdImageSY(this->fb_image)){
						this->image_idx_table.get()[idx++] = (unsigned int*)&this->fb_image->tpixels[y][x];
					}else{
						this->image_idx_table.get()[idx++] = NULL;
					}
				}
			}
		break;
		case R_180:
			for(int y = this->now_fbinfo.yres - 1; y >= 0; y--){
				for(int x = this->now_fbinfo.xres - 1; x >= 0; x--){
					if(x < gdImageSX(this->fb_image) && y < gdImageSY(this->fb_image)){
						this->image_idx_table.get()[idx++] = (unsigned int*)&this->fb_image->tpixels[y][x];
					}else{
						this->image_idx_table.get()[idx++] = NULL;
					}
				}
			}
		break;
		case R_270:
			for(int x = 0; x < this->now_fbinfo.yres; x++){
				for(int y = this->now_fbinfo.xres - 1; y >= 0; y--){
					if(x < gdImageSX(this->fb_image) && y < gdImageSY(this->fb_image)){
						this->image_idx_table.get()[idx++] = (unsigned int*)&this->fb_image->tpixels[y][x];
					}else{
						this->image_idx_table.get()[idx++] = NULL;
					}
				}
			}
		break;
		default:
			for(int y = 0; y < this->now_fbinfo.yres; y++){
				for(int x = 0; x < this->now_fbinfo.xres; x++){
					if(x < gdImageSX(this->fb_image) && y < gdImageSY(this->fb_image)){
						this->image_idx_table.get()[idx++] = (unsigned int*)&this->fb_image->tpixels[y][x];
					}else{
						this->image_idx_table.get()[idx++] = NULL;
					}
				}
			}
		break;
	}
}

void framebuffer::update()
{
	if(this->fb_image == NULL) return;

	this->init();

	__asm__ volatile (
		"mov r0, #0;"

		"1:"
		/* idxループ */

		/* ピクセルへのアドレスを生成 */
		"ldr r3, [%1, r0, lsl #2];"
		"ldr r2, [r3];"

		/* 変換 */
		"rev r2, r2;"
		"lsr r2, #8;"

		/* 保存 */
		"ldr r3, [%0, r0, lsl #2];"
		"tst r3, r3;"
		"strne r2, [r3];"

		"add r0, #1;"
		"teq r0, %2;"
		"bne 1b;"
		/* xループ終わり */

		:
		:"r"(this->image_idx_table.get()), "r"(this->fb_idx_table.get()), "r"(this->now_fbinfo.xres * this->now_fbinfo.yres)
		:"r0", "r1", "r2", "r3"
	);
}

void framebuffer::set_image(gdImagePtr image)
{
	this->fb_image = image;

	this->make_imgidx_table();
}

void framebuffer::set_rotate(FB_DEGREE degree)
{
	this->rotate = degree;

	int fb_width, fb_height;

	switch(this->rotate){
		case R_90:
		case R_270:
			fb_width = this->size.y;
			fb_height = this->size.x;
		break;
		default:
			fb_width = this->size.x;
			fb_height = this->size.y;
		break;
	}

	//フレームバッファのサイズ変更
	struct fb_var_screeninfo fbinfo_tmp;
	ioctl(framebuffer::fb_fd, FBIOGET_VSCREENINFO, &fbinfo_tmp);

	if(fbinfo_tmp.xres != fb_width || fbinfo_tmp.yres != fb_height ||
		fbinfo_tmp.xres_virtual != fb_width || fbinfo_tmp.yres_virtual != fb_height)
	{
		fbinfo_tmp.xres = fbinfo_tmp.xres_virtual = fb_width;
		fbinfo_tmp.yres = fbinfo_tmp.yres_virtual = fb_height;

		ioctl(framebuffer::fb_fd, FBIOPUT_VSCREENINFO, &fbinfo_tmp);

		this->init();
	}else{
		this->make_imgidx_table();
	}
}

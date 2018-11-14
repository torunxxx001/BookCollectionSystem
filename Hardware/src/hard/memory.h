#ifndef __MEMORY_H__
#define __MEMORY_H__

//RaspberryPi内のSDRAMの物理アドレス
#define SDRAM_PHSIC_ADDR	0x40000000

//sizeを4096の倍数に丸める
#define ROUND_TO_PAGE(size) (((size) + 4096 - 1) & ~(4096 - 1))


#ifdef __cplusplus
extern "C" {
#endif

unsigned long conv_virt_to_physic(void* virt_addr);
void* memalloc(int size);
void memfree(void* mem_addr, int size);
void* map_physic(unsigned long physic_address, int size);
void unmap_physic(void* mem_addr, int size);

#ifdef __cplusplus
}
#endif

#endif
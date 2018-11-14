#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "memory.h"

//自プロセスの仮想アドレスを物理アドレスに変換する関数
unsigned long conv_virt_to_physic(void* virt_addr)
{
	char pagemap_fn[128];
	int pid = getpid();
	sprintf(pagemap_fn, "/proc/%d/pagemap", pid);


	unsigned long long pfn;
	int fd = open(pagemap_fn, O_RDONLY);

	//9ビット右シフトは、割る512、つまり(x / 4096) * 8の変形
	lseek(fd, ((unsigned long)virt_addr >> 9) & ~0b111, SEEK_SET);
	read(fd, &pfn, sizeof(pfn));
	close(fd);

	//12ビット左シフトは、メモリフレームサイズが4096なため
	return (SDRAM_PHSIC_ADDR + ((unsigned long)pfn << 12) + ((unsigned long)virt_addr & 0xFFF));
}

void* memalloc(int size)
{
	void* map_addr = mmap(NULL, ROUND_TO_PAGE(size), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_LOCKED, -1, 0);

	return map_addr == MAP_FAILED ? NULL : map_addr;
}

void memfree(void* mem_addr, int size){
	munmap(mem_addr, ROUND_TO_PAGE(size));
}

void* map_physic(unsigned long physic_address, int size)
{
	int fd = open("/dev/mem", O_RDWR);
	void* map_addr = mmap(NULL, ROUND_TO_PAGE(size), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, fd, physic_address);
	close(fd);

	return map_addr == MAP_FAILED ? NULL : map_addr;
}

void unmap_physic(void* mem_addr, int size)
{
	munmap(mem_addr, ROUND_TO_PAGE(size));
}

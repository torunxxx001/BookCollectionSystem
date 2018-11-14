#ifndef __BARCODE_HPP__
#define __BARCODE_HPP__

#include <vector>
#include <string>
#include <pthread.h>


#define BARCODE_READER_ID "HID 03eb:6201"


#define KEY_COLON 40
#define KEY_ATMARK 26
#define KEY_LEFTBRACKET 27
#define KEY_RIGHTBRACKET 43
#define KEY_CARET 13
#define KEY_YEN_2 89


class barcode {
private:
	static int instance_count;

	//スレッド関連
	pthread_t thread_handle;
	int is_running;
	int terminate;

	//バーコードリスト
	static std::vector<std::string> barcode_list;
public:
	barcode();
	~barcode();

	void clear_barcode();
	std::string get_barcode();

	static void* barcode_thread(void* arg);
};


#endif

#include "barcode.hpp"

#include <linux/input.h>
#include <string.h>
#include <fcntl.h>
#include <linux/hiddev.h>


//バーコードリーダで使う文字テーブル
int code_table[] = {
	KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F,
	KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L,
	KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R,
	KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X,
	KEY_Y, KEY_Z, KEY_0, KEY_1, KEY_2, KEY_3,
	KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
	KEY_SPACE, KEY_COLON, KEY_SEMICOLON, KEY_COMMA, KEY_MINUS, KEY_DOT,
	KEY_SLASH, KEY_ATMARK, KEY_LEFTBRACKET, KEY_YEN, KEY_YEN_2, KEY_RIGHTBRACKET,
	KEY_CARET
};
char str_table_lower[] = {
	'a', 'b', 'c', 'd', 'e', 'f', 
	'g', 'h', 'i', 'j', 'k', 'l',
	'm', 'n', 'o', 'p', 'q', 'r',
	's', 't', 'u', 'v', 'w', 'x',
	'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9',
	' ', ':', ';', ',', '-', '.',
	'/', '@', '[', '\\', '\\', ']',
	'^'
};
char str_table_upper[] = {
	'A', 'B', 'C', 'D', 'E', 'F',
	'G', 'H', 'I', 'J', 'K', 'L',
	'M', 'N', 'O', 'P', 'Q', 'R',
	'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', '0', '!', '"', '#',
	'$', '%', '&', '\'', '(', ')',
	' ', '*', '+', '<', '=', '>',
	'?', '`', '{', '|', '_', '}',
	'~'
};

//クラスのstatic属性変数
int barcode::instance_count = 0;
std::vector<std::string> barcode::barcode_list;

barcode::barcode()
{
	if(this->instance_count == 0){
		this->is_running = 0;
		pthread_create(&this->thread_handle, NULL, &barcode::barcode_thread, (void*)this);

		//スレッドが始まるまで待機
		for(int i = 0; i < 1000; i++){
			if(this->is_running) break;
			usleep(100);
		}
	}
	++this->instance_count;
}

barcode::~barcode()
{
	--this->instance_count;
	if(this->instance_count == 0){
		//スレッド開始状態なら
		if(this->is_running){
			this->terminate = 1;
			pthread_join(this->thread_handle, NULL);
		}
	}
}

void barcode::clear_barcode()
{
	this->barcode_list.resize(0);
}

std::string barcode::get_barcode()
{
	if(this->barcode_list.size() > 0){
		std::string last_barcode = this->barcode_list.back();
		this->barcode_list.pop_back();

		return last_barcode;
	}

	return "";
}

void* barcode::barcode_thread(void* arg)
{
	barcode* bptr = (barcode*)arg;

	bptr->is_running = 1;
	bptr->terminate = 0;

	//bptr->terminateが0の間ループ
	do{
		int found_reader = 0;
		//バーコードリーダが見つかるまでループ
		int fd;
		//バーコードリーダを探す
		for(int index = 0;; index++){
			char buff[100];
			sprintf(buff, "/dev/input/event%d", index);

			fd = open(buff, O_RDONLY | O_NONBLOCK);
			if(fd == -1) break;

			char name[1024];
			ioctl(fd, EVIOCGNAME(1024), name);
			if(strcmp(name, BARCODE_READER_ID) == 0){
				found_reader = 1;
				break;
			}

			close(fd);
		}

		//バーコード読み取りループ
		if(found_reader) {
			fd_set rfds;
			fd_set fd_bak;
			struct timeval tm;

			FD_ZERO(&rfds);
			FD_SET(fd, &rfds);

			tm.tv_sec = 0;
			tm.tv_usec = 100000;
			memcpy(&fd_bak, &rfds, sizeof(fd_set));

			struct input_event event;

			int res_val;
			int upper_flag = 0;
			std::string barcode_buffer;
			while(bptr->terminate == 0 && (res_val = select(fd + 1, &rfds, NULL, NULL, &tm)) != -1){
				memcpy(&rfds, &fd_bak, sizeof(fd_set));
				tm.tv_usec = 100000;

				if(res_val > 0){
					if(read(fd, &event, sizeof(event)) == sizeof(event)){
						if(event.type == EV_KEY && event.value == 0){
							char tmp_char = '\0';

							if(event.code == KEY_RIGHTSHIFT || event.code == KEY_LEFTSHIFT){
								upper_flag = 1;
							}else{
								for(int c_index = 0; c_index < sizeof(code_table)/sizeof(int); c_index++){
									if(event.code == code_table[c_index]){
										if(upper_flag){
											tmp_char = str_table_upper[c_index];

											upper_flag = 0;
										}else{
											tmp_char = str_table_lower[c_index];
										}

										break;
									}
								}
							}

							if(event.code == KEY_ENTER){
								barcode_list.push_back(barcode_buffer);
								barcode_buffer = "";
							}else if(tmp_char != '\0'){
								barcode_buffer += tmp_char;
							}
						}
					}else{
						//読み取り失敗の場合はループ終了
						break;
					}
				}
			}

			close(fd);
		}

		//100ミリ秒のウェイト
		usleep(1000 * 100);
	}while(bptr->terminate == 0);

	bptr->terminate = 0;
	bptr->is_running = 0;

	pthread_exit(NULL);
}

#include "ex_display.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <signal.h>

#include "lib.hpp"

int terminate = 0;
void sigcatch(int signal)
{
	terminate = 1;
}

int main()
{
	//シグナルハンドラ設定
	signal(SIGINT, sigcatch);
	signal(SIGTERM, sigcatch);

	try{
		//ルートで実行されているかチェック
		if(getuid() != 0){
			throw std::string("ルート権限で実行してください");
		}

		std::string dir_path = lib::get_local_path();

		ex_display disp(dir_path + "conf/screen_objects.xml", dir_path + "conf/touch_setting.dat");
		disp.start();

		//PIDファイル作成
		std::string pid_file = "/var/run/book_sys.pid";

		//PID出力
		std::ofstream s_pid(pid_file);
		s_pid << (int)getpid();
		s_pid.close();

		do {
			usleep(100 * 1000);
		}while(terminate == 0 && disp.get_running_status());

		//PIDファイル削除
		system(("rm -f " + pid_file).c_str());
	}catch(std::string message){
		std::cerr << "error:" << message << std::endl;
	}

	return 0;
}
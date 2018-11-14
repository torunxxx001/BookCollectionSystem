#include "lib.hpp"

#include <sstream>

#include <stdio.h>
#include <unistd.h>

//実行ファイルのあるパスを取得するメソッド
std::string lib::get_local_path()
{
	std::string path;
	std::ostringstream oss;
	oss << "/proc/" << getpid() << "/exe";

	path.resize(1024);
	readlink(oss.str().c_str(), &(*path.begin()), path.length());

	int slash_pos = path.find_last_of('/');

	if(slash_pos == std::string::npos){
		return "";
	}

	return path.substr(0, path.find_last_of('/') + 1);
}

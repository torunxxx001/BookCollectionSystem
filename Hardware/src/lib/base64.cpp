#include "base64.hpp"

//base64文字テーブル
std::string str_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


std::string base64::encode(base64::BASE64_DATA data)
{
	std::string base64_code;

	//base64エンコードする処理
	unsigned char last = 0;
	for(int idx = 0; idx < data.size(); idx++){
		switch(idx % 3){
		case 0:
			base64_code += str_table[(data[idx] & 0xFC) >> 2];
			last = ((data[idx] & 0x03) << 4) | 0x80;
		break;
		case 1:
			base64_code += str_table[((data[idx - 1] & 0x03) << 4)
										| ((data[idx] & 0xF0) >> 4)];
			last = ((data[idx] & 0x0F) << 2) | 0x80;
		break;
		case 2:
			base64_code += str_table[((data[idx - 1] & 0x0F) << 2)
										| ((data[idx] & 0xC0) >> 6)];
			base64_code += str_table[data[idx] & 0x3F];
			last = 0;
		break;
		}
	}
	if(last > 0){
		base64_code += str_table[last & 0x3F];
	}
	//４文字単位に満たない最後の部分を=で埋める
	int remain = base64_code.length() % 4;
	base64_code.resize(base64_code.length() + remain, '=');

	//76文字ごとに改行を入れる
	for(int idx = 76; idx < base64_code.length(); idx += 76){
		base64_code.insert(idx, "\r\n", 2);
		idx += 2;
	}

	return base64_code;
}

base64::BASE64_DATA base64::decode(std::string base64_code)
{
	BASE64_DATA data;

	//base64デコードする処理
	for(int idx = 0; idx < base64_code.length(); idx+=4){
		//改行があったら飛ばす
		if(base64_code[idx] == '\r'){
			idx += 2;
		}
		if(base64_code[idx] == '\n'){
			idx += 1;
		}

		int pos1 = str_table.find(base64_code[idx]);
		int pos2 = str_table.find(base64_code[idx+1]);
		int pos3 = str_table.find(base64_code[idx+2]);
		int pos4 = str_table.find(base64_code[idx+3]);

		if(pos1 == std::string::npos || pos2 == std::string::npos) break;
		data.push_back((pos1 << 2) | ((pos2 & 0x30) >> 4));

		if(pos3 == std::string::npos) break;
		data.push_back(((pos2 & 0x0F) << 4) | ((pos3 & 0x3C) >> 2));

		if(pos4 == std::string::npos) break;
		data.push_back(((pos3 & 0x03) << 6) | (pos4 & 0x3F));
	}

	return data;
}
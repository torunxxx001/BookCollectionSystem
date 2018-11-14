#include "code128.hpp"

std::string spdecimal_table = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";

std::string code128::hashTo(bigcalc::BigDecimal hash)
{
	bigcalc::BigDecimal stbl_num = {(unsigned char)spdecimal_table.length()};

	std::string code_str;
	//バイナリからCODE128形式の文字列にする
	//hash値が0になるまで割っていく
	bigcalc::BigDecimal zero = {0};
	while( !bigcalc::check_equal(hash, zero)){
		std::vector<bigcalc::BigDecimal> result = bigcalc::div(hash, stbl_num);
		if(result.size() == 0){
			break;
		}
		hash = result[0];
		bigcalc::BigDecimal reminder = bigcalc::trim_zero(result[1]);

		code_str += spdecimal_table[reminder[0]];
	}

	return code_str;
}

bigcalc::BigDecimal code128::toHash(std::string code_str)
{
	bigcalc::BigDecimal hash = {0};
	bigcalc::BigDecimal stbl_num = {(unsigned char)spdecimal_table.length()};

	//CODE128形式からバイナリ文字にする
	int exec = 0;
	for(int idx = code_str.length() - 1; idx >= 0; idx--){
		char code_char = code_str[idx];

		int pos = spdecimal_table.find(code_str[idx]);
		if(pos != std::string::npos){
			hash = bigcalc::mul(hash, stbl_num);

			bigcalc::BigDecimal val_tmp = {(unsigned char)pos};
			hash = bigcalc::add(hash, val_tmp);

			exec = 1;
		}
	}

	//処理が実行されなければ戻り値の配列サイズを０に
	if( !exec){
		hash.resize(0);
	}

	return hash;
}

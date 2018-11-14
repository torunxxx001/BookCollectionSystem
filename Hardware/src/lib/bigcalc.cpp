#include "bigcalc.hpp"

#include <sstream>
#include <iomanip>
#include <algorithm>

//両方の桁数を揃えるメソッド
void bigcalc::adjust_digit_width(bigcalc::BigDecimal& a, bigcalc::BigDecimal& b)
{
	BigDecimal a_tmp = a;
	BigDecimal b_tmp = b;

	//少ない桁の方を多い桁に合わせる
	if(a.size() > b.size()){
		int diff = a.size() - b.size();
		b_tmp.resize(a.size());

		for(int i = 0; i < b_tmp.size(); i++){
			if(i >= diff){
				b_tmp[i] = b[i - diff];
			}else{
				b_tmp[i] = 0;
			}
		}
	}else if(a.size() < b.size()){
		int diff = b.size() - a.size();
		a_tmp.resize(b.size());

		for(int i = 0; i < a_tmp.size(); i++){
			if(i >= diff){
				a_tmp[i] = a[i - diff];
			}else{
				a_tmp[i] = 0;
			}
		}
	}

	a = a_tmp;
	b = b_tmp;
}

int bigcalc::left_shift(bigcalc::BigDecimal& a)
{
	if(a.size() == 0) return 0;

	int over = (a[0] >> 7) & 1;

	for(int i = 0; i < a.size() - 1; i++){
		a[i] = (a[i] << 1) | (a[i + 1] >> 7);
	}
	*(a.end() - 1) <<= 1;

	//最後にoverが１なら先頭に一桁追加
	if(over){
		a.insert(a.begin(), 1);
	}

	return over;
}

int bigcalc::right_shift(bigcalc::BigDecimal& a)
{
	if(a.size() == 0) return 0;

	int under = a.back() & 1;

	for(int i = a.size() - 1; i > 0; i--){
		a[i] = (a[i] >> 1) | ((a[i - 1]  & 1) << 7);
	}
	a[0] >>= 1;

	return under;
}

// a >= b チェック
int bigcalc::check_above(bigcalc::BigDecimal a, bigcalc::BigDecimal b)
{
	adjust_digit_width(a, b);

	BigDecimal result = sub(a, b);

	//引いた後の桁数でチェック
	if(result.size() == a.size()){
		//桁数が同じ=プラス
		return 1;
	}else{
		return 0;
	}
}

//a = bチェック
int bigcalc::check_equal(bigcalc::BigDecimal a, bigcalc::BigDecimal b)
{
	adjust_digit_width(a, b);

	BigDecimal result = sub(a, b);

	result = trim_zero(result);
	//引いた結果が０なら同じ
	if(result.size() == 1 && result[0] == 0){
		return 1;
	}else{
		return 0;
	}
}

//先頭の余分な０を削除するメソッド
bigcalc::BigDecimal bigcalc::trim_zero(bigcalc::BigDecimal a)
{
	BigDecimal result = a;

	while(result.size() > 1){
		if(result[0] > 0) break;

		result.erase(result.begin());
	}

	return result;
}

bigcalc::BigDecimal bigcalc::add(bigcalc::BigDecimal a, bigcalc::BigDecimal b)
{
	adjust_digit_width(a, b);

	BigDecimal result;

	unsigned char carry = 0;
	for(int i = a.size() - 1; i >= 0 || carry; i--){
		unsigned short val = 0;
		if(i >= 0){
			val = a[i] + b[i] + carry;
		}else{
			val = carry;
		}

		result.push_back((unsigned char)(val & 0xFF));
		carry = val >> 8;
	}
	std::reverse(result.begin(), result.end());

	return result;
}

bigcalc::BigDecimal bigcalc::sub(bigcalc::BigDecimal a, bigcalc::BigDecimal b)
{
	adjust_digit_width(a, b);

	//１の補数を作成
	for(int i = 0; i < b.size(); i++){
		b[i] = ~b[i];
	}
	//１の補数に１を足し２の補数に
	BigDecimal one_tmp = {1};
	b = add(b, one_tmp);

	//補数を足す
	BigDecimal result = add(a, b);

	//足した結果桁が変わらなかったらマイナスとする
	if(result.size() == a.size()){
		result.insert(result.begin(), 0xFF);
		result.insert(result.begin(), 0xFF);
	}else if(result.size() > a.size()){
		//桁が変わったらプラス
		//入力の桁数に戻す

		int diff = result.size() - a.size();
		result.erase(result.begin(), result.begin() + diff);
	}

	return result;
}

bigcalc::BigDecimal bigcalc::mul(bigcalc::BigDecimal a, bigcalc::BigDecimal b)
{
	adjust_digit_width(a, b);

	//幅をaに揃える
	BigDecimal sum_tmp;
	adjust_digit_width(a, sum_tmp);

	//サイズx8ビット分ループ
	int length = a.size() * 8;
	for(int i = 0; i < length; i++){
		//最下位ビットが１なら加算
		if((*(a.end() - 1)) & 1){
			sum_tmp = add(sum_tmp, b);
		}

		left_shift(b);
		right_shift(a);
	}

	return sum_tmp;
}

//返却値の0番目がquotientで1番目がreminder
std::vector<bigcalc::BigDecimal> bigcalc::div(bigcalc::BigDecimal a, bigcalc::BigDecimal b)
{
	std::vector<BigDecimal> result;

	adjust_digit_width(a, b);
	BigDecimal a_tmp = a;
	BigDecimal b_tmp = b;

	//徐数の先頭の余分な０を削除
	b_tmp = trim_zero(b_tmp);

	//0で割られていないかチェック
	if(b_tmp.size() == 1 && b_tmp[0] == 0) return result;

	//割る数を割られる数に揃える
	while(a_tmp.size() > b_tmp.size() || !(b_tmp[0] & 0x80)){
		left_shift(b_tmp);
	}

	BigDecimal quotient = {0};
	BigDecimal reminder = {0};
	while(1){
		int bit_val = 0;

		// a_tmpからbを引けるかチェック
		if(check_above(a_tmp, b_tmp)){
			//a_tmpからbを引いたものを代入
			a_tmp = sub(a_tmp, b_tmp);

			bit_val = 1;
		}

		//quotientの最下位ビットに追加
		*(quotient.end() - 1) |= bit_val;

		right_shift(b_tmp);
		//右シフトの結果、b_tmpが初期状態より小さくなれば終了
		if( !check_above(b_tmp, b)){
			break;
		}

		//quotientを左シフト
		left_shift(quotient);
	}
	reminder = a_tmp;

	result.push_back(quotient);
	result.push_back(reminder);

	return result;
}

std::string bigcalc::to_str(bigcalc::BigDecimal& a)
{
	std::ostringstream os_tmp;

	for(int i = 0; i < a.size(); i++){
		os_tmp << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (int)a[i];
	}

	return os_tmp.str();
}
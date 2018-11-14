#include "isbn.hpp"


int isbn::calcISBNdigit(std::string isbn)
{
	int digit = 0;

	//デジット計算
	if(isbn.length() == 10){
		int sum = 0;

		for(int idx = 0, v = 10; idx < isbn.length() - 1; idx++, v--){
			sum += (isbn[idx] % '0') * v;
		}

		digit = sum % 11;
		digit = 11 - digit;
		if(digit == 11) digit = 0;
	}else if(isbn.length() == 13){
		int sum = 0;

		for(int idx = 0, v = 1; idx < isbn.length() - 1; idx++){
			sum += (isbn[idx] % '0') * v;

			v = (v == 1 ? 3 : 1);
		}

		digit = sum % 10;
		digit = 10 - digit;
		if(digit == 10) digit = 0;
	}else{
		throw std::string("ISBNの長さが異常です");
	}

	return digit;
}

int isbn::checkISBN(std::string isbn)
{
	if(isbn.length() == 0) return isbn::ng;

	//デジット取り出し
	int digit = *(isbn.end() - 1);

	if(digit == 'X'){
		digit = 10;
	}else{
		digit %= '0';
	}

	int calc_digit;

	try {
		calc_digit = isbn::calcISBNdigit(isbn);
	}catch(...){
		return isbn::ng;
	}

	if(digit != calc_digit){
		return isbn::ng;
	}

	return isbn::ok;
}

std::string isbn::convertISBN10to13(std::string isbn10)
{
	if(checkISBN(isbn10) == isbn::ng){
		throw std::string("ISBNが異常です");
	}

	if(isbn10.length() == 13){
		//正しいISBNでなければエラー

		if(isbn10.find("978") == std::string::npos
			&& isbn10.find("979") == std::string::npos)
		{
			throw std::string("ISBN13が異常です");
		}

		return isbn10;
	}else if(isbn10.length() != 10){
		throw std::string("ISBNが異常です");
	}

	std::string result = "978" + isbn10;

	int digit = isbn::calcISBNdigit(result);
	*(result.end() - 1) = '0' + digit;

	return result;
}

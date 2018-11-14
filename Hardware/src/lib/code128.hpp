#ifndef __CODE128_HPP__
#define __CODE128_HPP__

#include "bigcalc.hpp"

#include <string>

class code128 {
public:
	static std::string hashTo(bigcalc::BigDecimal hash);
	static bigcalc::BigDecimal toHash(std::string code_str);
};


#endif

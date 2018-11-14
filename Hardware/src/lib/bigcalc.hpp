#ifndef __BIGCALC_HPP__
#define __BIGCALC_HPP__

#include <string>
#include <vector>

class bigcalc {
public:
	typedef std::vector<unsigned char> BigDecimal;

private:
	static void adjust_digit_width(BigDecimal& a, BigDecimal& b);

public:
	static int left_shift(BigDecimal& a);
	static int right_shift(BigDecimal& a);
	static int check_above(BigDecimal a, BigDecimal b);
	static int check_equal(BigDecimal a, BigDecimal b);

	static BigDecimal trim_zero(BigDecimal a);

	static BigDecimal add(BigDecimal a, BigDecimal b);
	static BigDecimal sub(BigDecimal a, BigDecimal b);
	static BigDecimal mul(BigDecimal a, BigDecimal b);
	static std::vector<BigDecimal> div(BigDecimal a, BigDecimal b);

	static std::string to_str(BigDecimal& a);
};

#endif

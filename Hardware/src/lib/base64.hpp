#ifndef __BASE64_HPP__
#define __BASE64_HPP__

#include <vector>
#include <string>

class base64 {
public:
	typedef std::vector<unsigned char> BASE64_DATA;

	static std::string encode(BASE64_DATA data);
	static BASE64_DATA decode(std::string base64_code);
};

#endif

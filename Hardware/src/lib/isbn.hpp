#ifndef __ISBN_HPP__
#define __ISBN_HPP__

#include <string>

class isbn {
public:
	static const int ok = 1;
	static const int ng = -1;

private:
	static int calcISBNdigit(std::string isbn);

public:
	static int checkISBN(std::string isbn);
	static std::string convertISBN10to13(std::string isbn10);
};

#endif

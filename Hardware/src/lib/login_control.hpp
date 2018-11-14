#ifndef __LOGIN_CONTROL_HPP__
#define __LOGIN_CONTROL_HPP__

#include <map>
#include <string>

class login_control {
private:
	std::map<std::string, std::string> db_info;

	std::string user_id;
	int admin;

public:
	login_control(std::string db_host, std::string db_user, std::string db_password, std::string db_name);

	void login(std::string user_id, std::string password);
	void easy_login(std::string code_str);

	void logout();

	std::string get_login_user();
	int is_admin();
};

#endif

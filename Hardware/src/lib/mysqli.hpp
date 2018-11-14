#ifndef __MYSQLI_HPP__
#define __MYSQLI_HPP__

#include <memory>
#include <string>
#include <vector>

#include <mysql/mysql.h>

class mysqli {
public:
	typedef std::vector<char> MYSQLI_STR;

	class mysqli_result {
	public:
		typedef std::vector<std::string> MYSQLI_RES_FIELDS;
		typedef std::vector<mysqli::MYSQLI_STR> MYSQLI_RES_ROW;

	private:
		MYSQL_RES* sql_result;

		MYSQLI_RES_FIELDS field_list;
		MYSQLI_RES_ROW row_data;

		MYSQLI_RES_FIELDS get_table_fields();
	public:
		mysqli_result(MYSQL_RES* in_res);
		~mysqli_result();

		void free_result();

		my_ulonglong num_rows();
		void data_seek(my_ulonglong offset);

		my_ulonglong num_fields();
		MYSQLI_RES_FIELDS* fetch_fields();

		int fetch_row();
		mysqli::MYSQLI_STR* at(int key);
		mysqli::MYSQLI_STR* at(std::string key);
		mysqli::MYSQLI_STR* operator[](int key);
		mysqli::MYSQLI_STR* operator[](std::string key);
	};

	typedef std::shared_ptr<mysqli_result> MYSQLI_RES;
private:
	MYSQL* dbcon;

public:
	mysqli(std::string host, std::string user, std::string password, std::string dbname);
	~mysqli();

	void close();

	std::string error();

	MYSQLI_RES query(std::string in_sql);
	std::string real_escape_string(MYSQLI_STR input);

	static std::string mysqli_str_to_string(MYSQLI_STR& input);
	static MYSQLI_STR string_to_mysqli_str(std::string input);
};

#endif

#ifndef __BOOK_MANAGER_HPP__
#define __BOOK_MANAGER_HPP__

#include "mysqli.hpp"
#include "base64.hpp"

#include <map>
#include <string>

//バッファサイズ
#define READ_STRING_BUFFER_SIZE (1024 * 100)

//情報取得用PHPのファイル名前
#define GET_BOOK_INFO_PHP_NAME "get_book_info.php"
//同期実行用PHPファイルの名前
#define SYNC_SQL_EXEC_PHP_NAME "sync/exec_sync.php"

class book_manager {
public:
	typedef std::map<std::string, mysqli::MYSQLI_STR> BOOK_INFO;

private:
	std::map<std::string, std::string> db_info;

	void exec_sync(std::vector<base64::BASE64_DATA>& sql_list);
	void exec_sync(base64::BASE64_DATA& sql_str);

public:
	book_manager(std::string db_host, std::string db_user, std::string db_password, std::string db_name);

	void regist_book(std::string isbn);
	BOOK_INFO search_book(std::string isbn);

	void borrow_books(std::string user_id, std::vector<std::string> isbn_list);
	void return_books(std::vector<std::string> isbn_list);

	std::vector<std::string> get_borrow_books(std::string user_id);
};

#endif


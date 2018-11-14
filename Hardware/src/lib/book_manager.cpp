#include "book_manager.hpp"

#include "lib.hpp"
#include "isbn.hpp"

#include <map>
#include <boost/algorithm/string/join.hpp>

book_manager::book_manager(std::string db_host, std::string db_user, std::string db_password, std::string db_name)
{
	this->db_info["host"] = db_host;
	this->db_info["user"] = db_user;
	this->db_info["password"] = db_password;
	this->db_info["dbname"] = db_name;
}

void book_manager::exec_sync(std::vector<base64::BASE64_DATA>& sql_list)
{
	std::string sync_path = lib::get_local_path() + SYNC_SQL_EXEC_PHP_NAME;

	FILE* proc = popen(sync_path.c_str(), "w");

	for(int idx = 0; idx < sql_list.size(); idx++){
		//SQLアイテムをbase64エンコードした後に送信
		fputs((base64::encode(sql_list[idx]) + ",").c_str(), proc);
	}

	pclose(proc);
}

void book_manager::exec_sync(base64::BASE64_DATA& sql_str)
{
	std::string sync_path = lib::get_local_path() + SYNC_SQL_EXEC_PHP_NAME;

	FILE* proc = popen(sync_path.c_str(), "w");

	//SQLアイテムをbase64エンコードした後に送信
	fputs((base64::encode(sql_str) + ",").c_str(), proc);

	pclose(proc);
}

void book_manager::regist_book(std::string isbn)
{
	//同期用SQLリスト
	std::vector<base64::BASE64_DATA> sync_sql_list;
	base64::BASE64_DATA b64_data_tmp;

	//ISBNが正しいかチェック
	if(isbn::checkISBN(isbn) == isbn::ng){
		throw std::string("ISBNが正しくありません");
	}

	//図書情報を一時保管する連想配列
	std::map<std::string, std::string> book_info;

	//ISBNから図書情報を取得
	std::string php_path = lib::get_local_path() + GET_BOOK_INFO_PHP_NAME;

	FILE* proc = popen((php_path + " " + isbn).c_str(), "r");
	if(proc == NULL) throw std::string("図書情報取得用PHPの実行に失敗しました");

	std::string str_buff;
	str_buff.resize(READ_STRING_BUFFER_SIZE);

	int first = 0;
	while(fgets(&(*str_buff.begin()), str_buff.length(), proc)){
		//バッファの長さをNULL文字までに絞る
		int null_pos = str_buff.find('\0');
		if(null_pos != std::string::npos){
			str_buff.resize(null_pos - 1);
		}

		if(first == 0){
			//１行目がSUCCESS出なければエラー
			//改行コードを置き換える
			int pos = str_buff.find_last_of('\n');
			if(pos != std::string::npos){
				str_buff[pos] = '\0';
			}

			//SUCCESSかチェック
			if(str_buff != "SUCCESS"){
				pclose(proc);
				throw std::string("php:") +  str_buff;
			}

			first = 1;
		}else{
			//キーと値に分ける
			int pos = str_buff.find(':');

			if(pos != std::string::npos){
				std::string key = str_buff.substr(0, pos);
				std::string value = str_buff.substr(pos + 1, str_buff.length());

				book_info[key] += value;
			}
		}

		//バッファをクリア
		str_buff.resize(READ_STRING_BUFFER_SIZE);
		std::fill(str_buff.begin(), str_buff.end(), '\0');
	}

	pclose(proc);

	//DB接続
	mysqli dbcon(this->db_info["host"], this->db_info["user"], this->db_info["password"], this->db_info["dbname"]);

	//すでに図書が登録されている場合は削除して再作成
	std::string sql = "DELETE FROM book_list WHERE isbn = '" + isbn + "';";
	dbcon.query(sql);

	//同期用
	b64_data_tmp.resize(0);
	std::copy(sql.begin(), sql.end(), std::inserter(b64_data_tmp, b64_data_tmp.begin()));
	sync_sql_list.push_back(b64_data_tmp);

	sql = "DELETE FROM image_list WHERE isbn = '" + isbn + "';";
	dbcon.query(sql);

	//同期用
	b64_data_tmp.resize(0);
	std::copy(sql.begin(), sql.end(), std::inserter(b64_data_tmp, b64_data_tmp.begin()));
	sync_sql_list.push_back(b64_data_tmp);

	//図書情報をデーターベースに書き込む

	//image_data以外の文字をエスケープ
	std::map<std::string, std::string>::iterator it = book_info.begin();

	for(; it != book_info.end(); it++){
		std::string key = (*it).first;
		std::string data = (*it).second;

		if(key == "image_data") continue;

		book_info[key] = dbcon.real_escape_string(mysqli::string_to_mysqli_str(data));
	}

	//図書の基本情報を保存
	sql = "INSERT book_list VALUES ";

	sql += "('" + isbn + "', '" + book_info["title"] + "', '" + book_info["author"] +
			"', '" + book_info["publisher"] + "', '" + book_info["description"] + "', '" + book_info["genre"] +
			"', '" + book_info["salesrank"] + "', '" + book_info["binding"] + "', '" + book_info["language"] +
			"', '" + book_info["numberofpages"] + "', '" + book_info["price"] + "', '" + book_info["date"] +
			"', '" + book_info["image_url"] + "');";

	dbcon.query(sql);


	//同期用
	b64_data_tmp.resize(0);
	std::copy(sql.begin(), sql.end(), std::inserter(b64_data_tmp, b64_data_tmp.begin()));
	sync_sql_list.push_back(b64_data_tmp);


	//図書画像があれば図書画像を保存
	if(book_info["image_data"].length() > 0){
		base64::BASE64_DATA image_data;

		//base64からRAWデータにしてDBに保存
		image_data = base64::decode(book_info["image_data"]);

		mysqli::MYSQLI_STR tmp_str;
		std::copy(image_data.begin(), image_data.end(), std::inserter(tmp_str, tmp_str.begin()));

		//画像をDBに保存
		std::string sql;
		sql = "INSERT INTO image_list VALUES ('" + isbn + "', '" + dbcon.real_escape_string(tmp_str) + "');";

		dbcon.query(sql);


		//同期用
		b64_data_tmp.resize(0);
		std::copy(sql.begin(), sql.end(), std::inserter(b64_data_tmp, b64_data_tmp.begin()));
		sync_sql_list.push_back(b64_data_tmp);
	}

	//同期実行
	this->exec_sync(sync_sql_list);
}

book_manager::BOOK_INFO book_manager::search_book(std::string isbn)
{
	//ISBNが正しいかチェック
	if(isbn::checkISBN(isbn) == isbn::ng){
		throw std::string("ISBNが正しくありません");
	}

	//ISBNを元に図書情報を取得
	mysqli dbcon(this->db_info["host"], this->db_info["user"], this->db_info["password"], this->db_info["dbname"]);

	std::string sql = "SELECT * FROM book_list WHERE isbn = '" + isbn + "';";

	mysqli::MYSQLI_RES sql_res = dbcon.query(sql);

	//問い合わせに成功したら図書情報取り出し

	book_manager::BOOK_INFO book_info;
	if(sql_res != NULL){
		mysqli::mysqli_result::MYSQLI_RES_FIELDS* fields;
		fields = sql_res->fetch_fields();

		if(sql_res->fetch_row()){
			for(int idx = 0; idx < fields->size(); idx++){
				std::string field_name = fields->at(idx);

				book_info[field_name] = *(sql_res->at(field_name));
			}
		}

		//結果をクリア
		sql_res->free_result();
	}else{
		throw dbcon.error();
	}

	//次に図書画像を読取
	sql = "SELECT data FROM image_list WHERE isbn = '" + isbn + "';";

	sql_res = dbcon.query(sql);

	//問い合わせに成功したら図書画像取り出し
	if(sql_res != NULL){
		if(sql_res->fetch_row()){
			book_info["image"] = *(sql_res->at("data"));
		}

		//結果をクリア
		sql_res->free_result();
	}else{
		throw dbcon.error();
	}

	return book_info;
}

void book_manager::borrow_books(std::string user_id, std::vector<std::string> isbn_list)
{
	std::vector<std::string> ins_values;

	mysqli dbcon(this->db_info["host"], this->db_info["user"], this->db_info["password"], this->db_info["dbname"]);

	std::string esc_user_id = dbcon.real_escape_string(mysqli::string_to_mysqli_str(user_id));

	for(int idx = 0; idx < isbn_list.size(); idx++){
		std::string isbn = isbn_list[idx];

		//ISBNが正しいかチェックし正しければリストに入れる
		if(isbn::checkISBN(isbn) == isbn::ok){
			ins_values.push_back("('" + isbn + "', '" + esc_user_id + "', NOW())");
		}else{
			//正しくなければISBNリストから削除
			isbn_list.erase(isbn_list.begin() + idx);
		}
	}

	//ISBNが残れば
	if(ins_values.size() > 0){
		//一応挿入の前にダブルやつを削除しておく
		this->return_books(isbn_list);

		std::string sql = "INSERT INTO borrow_list VALUES " + boost::algorithm::join(ins_values, ",") + ";";

		dbcon.query(sql);

		//他ホストにもSQLを反映
		base64::BASE64_DATA data_tmp;
		std::copy(sql.begin(), sql.end(), std::inserter(data_tmp, data_tmp.begin()));

		this->exec_sync(data_tmp);
	}
}

void book_manager::return_books(std::vector<std::string> isbn_list)
{
	//ISBNをチェックし正しくないISBNを排除
	for(int idx = 0; idx < isbn_list.size(); idx++){
		std::string isbn = isbn_list[idx];

		if(isbn::checkISBN(isbn) == isbn::ng){
			isbn_list.erase(isbn_list.begin() + idx);
		}
	}

	//ISBNリストがまだ残っていればリストから削除
	if(isbn_list.size() > 0){
		mysqli dbcon(this->db_info["host"], this->db_info["user"], this->db_info["password"], this->db_info["dbname"]);

		//ISBNにシングルクオートをつける
		for(int idx = 0; idx < isbn_list.size(); idx++){
			isbn_list[idx] = "'" + isbn_list[idx] + "'";
		}

		std::string sql = "DELETE FROM borrow_list WHERE isbn IN (" + boost::algorithm::join(isbn_list, ",") + ");";

		dbcon.query(sql);

		//他ホストにもSQLを反映
		base64::BASE64_DATA data_tmp;
		std::copy(sql.begin(), sql.end(), std::inserter(data_tmp, data_tmp.begin()));

		this->exec_sync(data_tmp);
	}
}

std::vector<std::string> book_manager::get_borrow_books(std::string user_id)
{
	std::vector<std::string> result;

	mysqli dbcon(this->db_info["host"], this->db_info["user"], this->db_info["password"], this->db_info["dbname"]);

	std::string esc_user_id = dbcon.real_escape_string(mysqli::string_to_mysqli_str(user_id));

	//検索
	std::string sql = "SELECT isbn FROM borrow_list WHERE user_id = '" + esc_user_id + "';";

	mysqli::MYSQLI_RES sql_res = dbcon.query(sql);

	if(sql_res != NULL){
		//ISBNをリストに追加する
		while(sql_res->fetch_row()){
			mysqli::MYSQLI_STR* data = sql_res->at("isbn");
			if(data == NULL) continue;

			result.push_back(mysqli::mysqli_str_to_string(*data));
		}
	}else{
		throw dbcon.error();
	}

	return result;
}
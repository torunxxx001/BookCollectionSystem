#include "mysqli.hpp"

mysqli::mysqli(std::string host, std::string user, std::string password, std::string dbname)
{
	this->dbcon = mysql_init(NULL);
	if(this->dbcon == NULL){
		throw std::string("MySQLオブジェクトの確保に失敗しました");
	}

	//MySQLへ接続を試みる
	MYSQL* ret = mysql_real_connect(this->dbcon, host.c_str(), user.c_str(), password.c_str(), dbname.c_str(), 0, NULL, 0);
	if(ret == NULL){
		std::string error_message = mysql_error(this->dbcon);
		mysql_close(this->dbcon);

		//接続失敗の場合エラーを返す
		throw this->error();
	}

	//キャラクタセット変更
	mysql_set_character_set(this->dbcon, "utf8");
}

mysqli::~mysqli()
{
	if(this->dbcon != NULL){
		mysql_close(this->dbcon);
		this->dbcon = NULL;
	}
}

void mysqli::close()
{
	if(this->dbcon != NULL){
		mysql_close(this->dbcon);
		this->dbcon = NULL;
	}
}

std::string mysqli::error()
{
	return mysql_error(this->dbcon);
}

mysqli::MYSQLI_RES mysqli::query(std::string in_sql)
{
	int sql_result = mysql_query(this->dbcon, in_sql.c_str());
	if(sql_result != 0){
		throw "SQL実行エラー:" + this->error();
	}

	MYSQL_RES* res_tmp = mysql_use_result(this->dbcon);
	if(res_tmp == NULL){
		return NULL;
	}

	return MYSQLI_RES(new mysqli_result(res_tmp));
}

std::string mysqli::real_escape_string(mysqli::MYSQLI_STR input)
{
	std::string dest;

	dest.resize(input.size() * 2 + 1);
	unsigned long dest_len = mysql_real_escape_string(this->dbcon, &(*dest.begin()), &(*input.begin()), input.size());

	dest.resize(dest_len);

	return dest;
}

std::string mysqli::mysqli_str_to_string(mysqli::MYSQLI_STR& input)
{
	std::string result;

	std::copy(input.begin(), input.end(), std::inserter(result, result.begin()));

	return result;
}

mysqli::MYSQLI_STR mysqli::string_to_mysqli_str(std::string input)
{
	mysqli::MYSQLI_STR result;

	std::copy(input.begin(), input.end(), std::inserter(result, result.begin()));

	return result;
}


mysqli::mysqli_result::mysqli_result(MYSQL_RES* in_res)
{
	this->sql_result = in_res;

	this->field_list = this->get_table_fields();
}

mysqli::mysqli_result::~mysqli_result()
{
	if(this->sql_result != NULL){
		mysql_free_result(this->sql_result);
		this->sql_result = NULL;
	}
}

void mysqli::mysqli_result::free_result()
{
	if(this->sql_result != NULL){
		mysql_free_result(this->sql_result);
		this->sql_result = NULL;
	}
}

my_ulonglong mysqli::mysqli_result::num_rows()
{
	if(this->sql_result != NULL){
		return mysql_num_rows(this->sql_result);
	}

	return 0;
}

void mysqli::mysqli_result::data_seek(my_ulonglong offset)
{
	if(this->sql_result != NULL){
		if(offset >= 0 && offset < this->num_rows()){
			mysql_data_seek(this->sql_result, offset);
		}
	}
}

mysqli::mysqli_result::MYSQLI_RES_FIELDS mysqli::mysqli_result::get_table_fields()
{
	MYSQLI_RES_FIELDS tmp_fields;

	if(this->sql_result != NULL){
		my_ulonglong field_num = mysql_num_fields(this->sql_result);
		MYSQL_FIELD* fields = mysql_fetch_fields(this->sql_result);

		for(int idx = 0; idx < field_num; idx++){
			tmp_fields.push_back(fields[idx].name);
		}
	}

	return tmp_fields;
}

my_ulonglong mysqli::mysqli_result::num_fields()
{
	if(this->sql_result != NULL){
		return this->field_list.size();
	}

	return 0;
}

mysqli::mysqli_result::MYSQLI_RES_FIELDS* mysqli::mysqli_result::fetch_fields()
{
	if(this->sql_result != NULL){
		return &(this->field_list);
	}

	return NULL;
}

int mysqli::mysqli_result::fetch_row()
{
	this->row_data.resize(0);

	if(this->sql_result != NULL){
		MYSQL_ROW row_tmp = mysql_fetch_row(sql_result);
		if(row_tmp == NULL){
			return 0;
		}

		//現在の行のそれぞれの列長を取得
		unsigned long* lengths = mysql_fetch_lengths(sql_result);

		//mysqli_resultクラス内の行データを更新
		for(int idx = 0; idx < this->field_list.size(); idx++){
			mysqli::MYSQLI_STR data_tmp;

			std::copy(row_tmp[idx], row_tmp[idx] + lengths[idx], std::inserter(data_tmp, data_tmp.begin()));
			this->row_data.push_back(data_tmp);
		}

		return 1;
	}

	return 0;
}

mysqli::MYSQLI_STR* mysqli::mysqli_result::at(int key)
{
	if(key >= 0 && key < this->row_data.size()){
		return &(this->row_data[key]);
	}

	return NULL;
}

mysqli::MYSQLI_STR* mysqli::mysqli_result::at(std::string key)
{
	//キーの場所を探す
	int pos = -1;
	for(int idx = 0; idx < this->field_list.size(); idx++){
		if(this->field_list[idx] == key){
			pos = idx;
			break;
		}
	}

	if(pos >= 0 && pos < this->row_data.size()){
		return &(this->row_data[pos]);
	}

	return NULL;
}

mysqli::MYSQLI_STR* mysqli::mysqli_result::operator[](int key)
{
	return this->at(key);
}

mysqli::MYSQLI_STR* mysqli::mysqli_result::operator[](std::string key)
{
	return this->at(key);
}

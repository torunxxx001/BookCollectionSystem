#include "login_control.hpp"

#include "mysqli.hpp"
#include "code128.hpp"

login_control::login_control(std::string db_host, std::string db_user, std::string db_password, std::string db_name)
{
	this->db_info["host"] = db_host;
	this->db_info["user"] = db_user;
	this->db_info["password"] = db_password;
	this->db_info["dbname"] = db_name;

	this->user_id = "";
	this->admin = 0;
}

void login_control::login(std::string user_id, std::string password)
{
	//ログインできるかデータベースに問い合わせ
	mysqli dbcon(this->db_info["host"], this->db_info["user"], this->db_info["password"], this->db_info["dbname"]);

	std::string esc_user_id = dbcon.real_escape_string(mysqli::string_to_mysqli_str(user_id));
	std::string esc_password = dbcon.real_escape_string(mysqli::string_to_mysqli_str(password));

	std::string sql_str = "SELECT admin FROM user_list WHERE user_id = '" + esc_user_id + "' AND SHA1('" + esc_password + "') = password;";
	mysqli::MYSQLI_RES sql_res = dbcon.query(sql_str);

	if(sql_res != NULL){
		//ログインに成功=行が取れたら
		if(sql_res->fetch_row()){
			mysqli::MYSQLI_STR* admin = sql_res->at("admin");

			//ログインに成功したらログイン情報をセット
			this->user_id = user_id;

			//管理者かどうかセット
			this->admin = 0;
			if(admin && admin->size() > 0){
				if(admin->at(0) == '1'){
					this->admin = 1;
				}
			}
		}else{
			throw std::string("ログインに失敗しました");
		}
	}else{
		throw std::string("error:") + dbcon.error();
	}
}

void login_control::easy_login(std::string code_str)
{
	//CODE128からハッシュを抜き出す
	bigcalc::BigDecimal hash = code128::toHash(code_str);

	if(hash.size() == 0 || hash.size() != 17 || hash[0] != 0xFF){
		throw std::string("ログインバーコードが正しくありません");
	}

	//ハッシュ値を16桁にする
	hash.erase(hash.begin());

	//ハッシュ値を文字列に
	std::string hash_str = bigcalc::to_str(hash);

	//ログインできるかデータベースに問い合わせ
	mysqli dbcon(this->db_info["host"], this->db_info["user"], this->db_info["password"], this->db_info["dbname"]);

	std::string sql_str = "SELECT user_id, admin FROM user_list WHERE CAST(MD5(SHA1(CONCAT(user_id, password))) AS CHAR) LIKE '" + hash_str + "';";
	mysqli::MYSQLI_RES sql_res = dbcon.query(sql_str);

	if(sql_res != NULL){
		//ログインに成功=行が取れたら
		if(sql_res->fetch_row()){
			mysqli::MYSQLI_STR* user_id = sql_res->at("user_id");
			mysqli::MYSQLI_STR* admin = sql_res->at("admin");

			//ログインに成功したらログイン情報をセット
			this->user_id = "";
			if(user_id && user_id->size() > 0){
				this->user_id = mysqli::mysqli_str_to_string(*user_id);
			}else{
				throw std::string("user_idの取得に失敗しました");
			}

			//管理者かどうかセット
			this->admin = 0;
			if(admin && admin->size() > 0){
				if(admin->at(0) == '1'){
					this->admin = 1;
				}
			}
		}else{
			throw std::string("ログインバーコードにマッチするユーザが存在しません");
		}
	}else{
		throw std::string("error:") + dbcon.error();
	}
}

void login_control::logout()
{
	this->user_id = "";
	this->admin = 0;
}

std::string login_control::get_login_user()
{
	return this->user_id;
}

int login_control::is_admin()
{
	return this->admin;
}
#include "events.hpp"

#include "lib.hpp"
#include "barcode.hpp"
#include "xml_lib.hpp"
#include "login_control.hpp"
#include "book_manager.hpp"
#include "isbn.hpp"

#include "base.hpp"

#include <iomanip>
#include <memory>
#include <map>
#include <string>
#include <functional>
#include <iostream>
#include <sstream>
#include <boost/any.hpp>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h> 

//バーコード読み取り用クラス
barcode barcode_reader;

//ログイン情報格納用クラス
std::unique_ptr<login_control> login_con;

//図書情報操作用クラス
std::unique_ptr<book_manager> book_man;

//汎用的な関数が入る変数
std::map<std::string, std::function<void (boost::any)>> pub_funcs;

//汎用的な値が入る変数
std::map<std::string, boost::any> pub_vars;

//追加可能なイベント
//screen_containerのcreateとdestroy
//screenのloadとunloadとinterval
//objectのpushupとpushdown
void registEvents(screen_container* screens)
{
	screen::ScreenPtr scr_ptr;
	std::function<void (scr_object*)> event_func;


	//このイベントは描画開始時に呼び出される
	screens->set_create_extra([](){
		std::map<std::string, std::string> db_con;

		//データベース設定を読み取る
		c_xml db_setting(lib::get_local_path() + "conf/db_setting.xml");

		c_xml::TAG_LIST_PTR root = db_setting.get_root();

		if(root->size() == 0) throw std::string("データベース設定ファイルの内部構造が異常です");
		for(int idx = 0; idx < root->at(0).child_elements.size(); idx++){
			TAG_ELEMENT* tag_ptr = &(root->at(0).child_elements[idx]);

			db_con[tag_ptr->tag_name] = tag_ptr->value;
		}

		//データベース情報を使用するクラスへ設定を与える
		login_con = std::unique_ptr<login_control>(
			new login_control(db_con["host"], db_con["user"], db_con["password"], db_con["dbname"]));
		book_man = std::unique_ptr<book_manager>(
			new book_manager(db_con["host"], db_con["user"], db_con["password"], db_con["dbname"]));


		//FB描画時のコマンド送信用コンソールを開く
		pub_vars["fb_fd"] = open("/dev/console", O_RDWR | O_NOCTTY);

		//キーパッド関連の汎用変数
		pub_vars["keypad_return_id"] = std::string("");
		pub_vars["keypad_setstr_object"] = (scr_object*)NULL;
	});

	//このイベントは描画終了時に呼び出される
	screens->set_destroy_extra([](){
		//開いたコンソールを閉じる
		int fb_fd = boost::any_cast<int>(pub_vars["fb_fd"]);

		if(fb_fd >= 0){
			close(fb_fd);
		}
	});

	//screen_containerのselectイベント登録
	screens->set_select_extra([screens](screen* scr){

		//通常通知を行うときの関数
		pub_funcs["MessageBox"] = [scr](boost::any param){
			std::string message = boost::any_cast<std::string>(param);

			scr->get_object("MESSAGE")->set_attr("text", message);


			scr->get_object("MESSAGE")->set_visible(1);
			scr->get_object("MSGOK")->set_visible(1);
		};

		//トップ画面へボタンが付いた通知を行う関数
		pub_funcs["MessageBoxGoTop"] = [scr](boost::any param){
			std::string message = boost::any_cast<std::string>(param);

			scr->get_object("MESSAGE")->set_attr("text", message);

			scr->get_object("MESSAGE")->set_visible(1);
			scr->get_object("MSG_GOTOP")->set_visible(1);
		};

		//図書情報を表示する際の関数
		//通常通知を行うときの関数
		pub_funcs["ShowBookInfo"] = [scr](boost::any param){
			std::string message = boost::any_cast<std::string>(param);

			scr->get_object("MESSAGE")->set_attr("text", message);

			scr->get_object("MESSAGE")->set_visible(1);
			scr->get_object("MSGOK")->set_visible(1);
			scr->get_object("COVER_IMG")->set_visible(1);
		};

		//OKボタンがクリックされたらダイアログを閉じる
		scr->get_object("MSGOK")->set_pushup_extra([](scr_object* obj){
			obj->get_parent()->get_object("MESSAGE")->set_visible(0);
			obj->get_parent()->get_object("MSGOK")->set_visible(0);

			//COVER_IMGも非表示にしておく
			obj->get_parent()->get_object("COVER_IMG")->set_visible(0);
		});

		//トップ画面へボタンがクリックされたらトップ画面へ遷移
		scr->get_object("MSG_GOTOP")->set_pushup_extra([screens](scr_object* obj){
			//ダイアログを閉じる
			obj->get_parent()->get_object("MESSAGE")->set_visible(0);
			obj->get_parent()->get_object("MSG_GOTOP")->set_visible(0);

			//トップ画面へ戻る
			if(login_con->is_admin()){
				screens->select("TOP_ADMIN");
			}else{
				screens->select("TOP_NORMAL");
			}
		});
	});

	//screen_containerのloadedイベント登録
	screens->set_loaded_extra([](screen* scr){
		std::string scr_id = scr->get_id();

		//キーパッド戻りに関するセット
		//戻りか判断(KEYBOARDのPAD,LOWER,UPPER,SYMBOL以外なら実行)
		if(scr_id != "KEYBOARD_PAD" && scr_id != "KEYBOARD_LOWER"
			&& scr_id != "KEYBOARD_UPPER" && scr_id != "KEYBOARD_SYMBOL")
		{
			pub_vars["keypad_return_id"] = std::string(""); //クリア
		}
	});

	//---LOGINスクリーンイベント登録
	scr_ptr = screens->get_screen("LOGIN");

	//LOGINの初期実行イベント登録
	scr_ptr->set_load_extra([screens](screen* scr){
		//EasyBarcode用配列クリア
		pub_vars["EasyBarcode"] = std::vector<std::string>();

		//入力パッド戻り以外なら実行
		if(boost::any_cast<std::string>(pub_vars["keypad_return_id"]) != scr->get_id()){
			//USER_IDとPASSWORDフォームクリア
			scr->get_object("USER_ID")->set_attr("text", std::string(""));
			scr->get_object("PASSWORD")->set_attr("text", std::string(""));
		}
	});

	//LOINGの定期実行イベント登録
	scr_ptr->set_interval_extra([screens](screen* scr){
		//EasyLogin用バーコード読み取り検出
		std::string barcode = barcode_reader.get_barcode();

		//バーコードが読み取られていれば
		if(barcode != ""){
			std::vector<std::string> bar_list = boost::any_cast<std::vector<std::string> >(pub_vars["EasyBarcode"]);

			if(barcode[0] != '-' && barcode[0] != '*'){
				pub_vars["EasyBarcode"] = std::vector<std::string>();
				pub_funcs["MessageBox"](std::string("ログインバーコードが無効です, 初めから読み取ってください"));
			}else{
				bar_list.push_back(barcode);

				pub_vars["EasyBarcode"] = bar_list;
			}

			if(bar_list.size() >= 2){
				//EasyBarcodeクリア
				pub_vars["EasyBarcode"] = std::vector<std::string>();

				if((bar_list[0][0] == '-' && bar_list[1][0] == '-')
					|| (bar_list[0][0] == '*' && bar_list[1][0] == '*'))
				{
					pub_funcs["MessageBox"](std::string("識別子が同じバーコードがあります, 初めから読み取ってください"));
				}else{
					std::string var_tmp;
					if(bar_list[0][0] == '-'){
						var_tmp = bar_list[0].substr(1) + bar_list[1].substr(1);
					}else{
						var_tmp = bar_list[1].substr(1) + bar_list[0].substr(1);
					}

					try {
						login_con->easy_login(var_tmp);

						//ログイン成功後画面遷移
						if(login_con->is_admin()){
							screens->select("TOP_ADMIN");
						}else{
							screens->select("TOP_NORMAL");
						}
					} catch(std::string message) {
						pub_funcs["MessageBox"](message);
					}
				}
			}
		}
	});

	//ログインボタンのイベント登録
	scr_ptr->get_object("LOGIN_BUTTON")->set_pushup_extra([screens](scr_object* obj){
		//USER_IDとパスワードの取得
		boost::any* user_id = obj->get_parent()->get_object("USER_ID")->get_attr("text");
		boost::any* password = obj->get_parent()->get_object("PASSWORD")->get_attr("text");

		if(user_id && password){
			try{
				//ログインを試みる
				login_con->login(boost::any_cast<std::string>(*user_id), boost::any_cast<std::string>(*password));

				//ログイン成功後画面遷移
				if(login_con->is_admin()){
					screens->select("TOP_ADMIN");
				}else{
					screens->select("TOP_NORMAL");
				}
			}catch(std::string message){
				pub_funcs["MessageBox"](message);
			}
		}
	});


	//LOGINのテキストボックスタッチのイベント登録
	//イベント実態
	event_func = [screens](scr_object* obj){
		std::string scr_id = obj->get_parent()->get_id();

		//戻り先スクリーンID格納
		pub_vars["keypad_return_id"] = scr_id;

		//文字設定用オブジェクトセット
		pub_vars["keypad_setstr_object"] = obj;

		//キーボードへ遷移
		screens->select("KEYBOARD_LOWER");
	};
	scr_ptr->get_object("USER_ID")->set_pushup_extra(event_func);
	scr_ptr->get_object("PASSWORD")->set_pushup_extra(event_func);


	//--TOP画面関連のイベント
	std::vector<std::string> top_scr_names = {
		"TOP_NORMAL", "TOP_ADMIN"
	};

	//TOPのNORMALとADMINのイベント登録
	for(int idx = 0; idx < top_scr_names.size(); idx++){
		screen::ScreenPtr scr = screens->get_screen(top_scr_names[idx]);

		//画面ロード時
		scr->set_load_extra([](screen* scr){
			scr->get_object("L_FORM_STATUS")->set_attr("text", login_con->get_login_user());
		});

		//貸出ボタン
		scr->get_object("MENU_BORROW")->set_pushup_extra([screens](scr_object* obj){
			screens->select("BORROW_SCR");
		});
		//返却ボタン
		scr->get_object("MENU_RETURN")->set_pushup_extra([screens](scr_object* obj){
			screens->select("RETURN_SCR");
		});
		//確認ボタン
		scr->get_object("MENU_CHECK")->set_pushup_extra([screens](scr_object* obj){
			screens->select("CHECK_SCR");
		});
		//ログアウトボタン
		scr->get_object("MENU_LOGOUT")->set_pushup_extra([screens](scr_object* obj){
			//ログアウトする
			login_con->logout();

			//画面をログイン画面へ
			screens->select("LOGIN");
		});

		//管理者用
		if(top_scr_names[idx] == "TOP_ADMIN"){
			scr->get_object("MENU_BREGIST")->set_pushup_extra([screens](scr_object* obj){
				screens->select("BREGIST_SCR");
			});

			//コンソール表示ボタン
			scr->get_object("SHOW_CONSOLE")->set_pushup_extra([screens](scr_object* obj){
				screens->select("FRAMEBUFFER");
			});
		}
	}

	//--BORROW_SCR,RETURN_SCR,CHECK_SCR,BREGIST_SCR共通の処理を書く
	std::vector<std::string> bookcon_scr_names = {
		"BORROW_SCR", "RETURN_SCR", "CHECK_SCR", "BREGIST_SCR"
	};
	for(int idx = 0; idx < bookcon_scr_names.size(); idx++){
		screen::ScreenPtr scr = screens->get_screen(bookcon_scr_names[idx]);

		//画面ロード時
		scr->set_load_extra([](screen* scr){
			//ログインIDセット
			scr->get_object("L_FORM_STATUS")->set_attr("text", login_con->get_login_user());

			//キーパッド戻りなら実行しない
			if(boost::any_cast<std::string>(pub_vars["keypad_return_id"]) != scr->get_id()){
				//ISBNリスト
				pub_vars["book_list"] = std::vector<std::string>();
				//前リスト数
				pub_vars["old_book_list_num"] = 0;

				//現在のページ数
				pub_vars["book_list_cur_page"] = 0;
				//古いページ数
				pub_vars["book_list_old_page"] = 0;

				//その他の要素のアイテム再描画フラグ
				pub_vars["book_list_reset"] = 0;

				//表示状態設定
				for(int idx = 1; idx <= 5; idx++){
					std::ostringstream ofs_tmp;

					ofs_tmp << idx;

					//表示設定はCHECK_SCR以外実行
					if(scr->get_id() != "CHECK_SCR"){
						//EXPLAIN設定
						scr->get_object("EXPLAIN" + ofs_tmp.str())->set_visible(1);
						//ITEM設定
						scr->get_object("ITEM" + ofs_tmp.str())->set_visible(0);
						//IDEL設定
						scr->get_object("IDEL" + ofs_tmp.str())->set_visible(0);
						//INFO設定
						scr->get_object("INFO" + ofs_tmp.str())->set_visible(0);
					}

					//ITEMの中身クリア
					scr->get_object("ITEM" + ofs_tmp.str())->set_attr("text", std::string(""));
				}
				//ENTERを非表示に
				scr->get_object("ENTER")->set_visible(0);

				//ページ番号リセット
				scr->get_object("PAGE_NUM")->set_attr("text", std::string("01 / 01"));

				//CHECK_SCRのみの処理
				if(scr->get_id() == "CHECK_SCR"){
					//借りている図書の一覧を取得
					pub_vars["book_list"] = book_man->get_borrow_books(login_con->get_login_user());
				}
			}else{
				//キーパッドの戻りで実行する処理
				//戻りのオブジェクト
				scr_object* ret_obj = boost::any_cast<scr_object*>(pub_vars["keypad_setstr_object"]);

				//オブジェクト文字
				boost::any* obj_text = ret_obj->get_attr("text");

				if(obj_text){
					std::string isbn = boost::any_cast<std::string>(*obj_text);
					std::vector<std::string> isbn_list_tmp 
								= boost::any_cast<std::vector<std::string> >(pub_vars["book_list"]);

					try{
						//一応ISBN10から13に変換
						isbn = isbn::convertISBN10to13(isbn);

						//リストに既にあるかチェック
						int found = 0;
						for(int idx = 0; idx < isbn_list_tmp.size(); idx++){
							if(isbn == isbn_list_tmp[idx]){
								found = 1;
								break;
							}
						}

						if( !found){
							if(scr->get_id() == "BREGIST_SCR"){
								//新規図書登録
								try {
									//新規図書を登録する
									book_man->regist_book(isbn);

									//リストにISBNを追加
									isbn_list_tmp.push_back(isbn);
								} catch (std::string message){
									//エラーならITEM欄もクリア
									ret_obj->set_attr("text", std::string(""));
									//クリアしたので再セット
									pub_vars["book_list_reset"] = 1;

									pub_funcs["MessageBox"]("[" + isbn + "]" + message);
								}
							}else{
								try{
									//既存図書セット
									book_manager::BOOK_INFO book_info = book_man->search_book(isbn);

									//図書が見つかれば
									if(book_info.size() > 0){
										//リストにISBNを追加
										isbn_list_tmp.push_back(isbn);
									}else{
										pub_funcs["MessageBox"]("[" + isbn + "]" + std::string("図書が見つかりません"));
									}
								}catch(std::string message){
									//エラーならITEM欄もクリア
									ret_obj->set_attr("text", std::string(""));
									//クリアしたので再セット
									pub_vars["book_list_reset"] = 1;

									pub_funcs["MessageBox"]("[" + isbn + "]" + message);
								}
							}
							pub_vars["book_list"] = isbn_list_tmp;

							//現在のページ数を最後に設定
							pub_vars["book_list_cur_page"] = (isbn_list_tmp.size() == 0 ? 0 : (ROUND_UP(isbn_list_tmp.size() / 5.0) - 1));
						}
					}catch(std::string message){
						//エラーならITEM欄もクリア
						ret_obj->set_attr("text", std::string(""));
						//クリアしたので再セット
						pub_vars["book_list_reset"] = 1;

						pub_funcs["MessageBox"]("[" + isbn + "]" + message);
					}
				}
			}
		});

		//定期実行
		scr->set_interval_extra([](screen* scr){
			std::string scr_id = scr->get_id();

			//ISBNリスト取得
			std::vector<std::string> isbn_list_tmp = boost::any_cast<std::vector<std::string> >(pub_vars["book_list"]);

			//CHECK_SCR以外実行
			if(scr_id != "CHECK_SCR"){
				std::string barcode = barcode_reader.get_barcode();

				if(barcode != ""){
					try{
						//一応ISBN10から13に変換
						barcode = isbn::convertISBN10to13(barcode);

						//リストに既にあるかチェック
						int found = 0;
						for(int idx = 0; idx < isbn_list_tmp.size(); idx++){
							if(barcode == isbn_list_tmp[idx]){
								found = 1;
								break;
							}
						}

						if( !found){
							if(scr_id == "BREGIST_SCR"){
								//新規図書登録
								try {
									//新規図書を登録する
									book_man->regist_book(barcode);

									//リストにISBNを追加
									isbn_list_tmp.push_back(barcode);
								} catch (std::string message){
									pub_funcs["MessageBox"](message);
								}
							}else{
								try{
									//既存図書セット
									book_manager::BOOK_INFO book_info = book_man->search_book(barcode);

									//図書が見つかれば
									if(book_info.size() > 0){
										//リストにISBNを追加
										isbn_list_tmp.push_back(barcode);
									}else{
										pub_funcs["MessageBox"]("[" + barcode + "]" + std::string("図書が見つかりません"));
									}
								}catch(std::string message){
									pub_funcs["MessageBox"]("[" + barcode + "]" + message);
								}
							}

							//現在のページ数を最後に設定
							pub_vars["book_list_cur_page"] = (isbn_list_tmp.size() == 0 ? 0 : (ROUND_UP(isbn_list_tmp.size() / 5.0) - 1));
						}
					}catch(std::string message){
						pub_funcs["MessageBox"]("[" + barcode + "]" + message);
					}
				}
			}

			//ITEM欄に実際に文字を反映させる
			int old_book_list_num = boost::any_cast<int>(pub_vars["old_book_list_num"]);

			int cur_page = boost::any_cast<int>(pub_vars["book_list_cur_page"]);
			int old_page = boost::any_cast<int>(pub_vars["book_list_old_page"]);

			int reset_flag = boost::any_cast<int>(pub_vars["book_list_reset"]);

			//リスト数が変わるかページ数が変わるか再描画フラグがあれば実行
			if(old_book_list_num != isbn_list_tmp.size() || cur_page != old_page || reset_flag){
				//要素数が変わったら

				//一応EXPLAIN1にタッチイベントを発生させる
				scr->get_object("EXPLAIN1")->call_pushup_extra();

				for(int idx = 1; idx <= 5; idx++){
					std::ostringstream ofs_tmp;
					ofs_tmp << idx;

					//ISBN取得
					int list_ptr = cur_page * 5 + (idx - 1);
					if(list_ptr < isbn_list_tmp.size()){
						std::string isbn = isbn_list_tmp[list_ptr];

						try{
							//図書情報取得
							book_manager::BOOK_INFO book_info = book_man->search_book(isbn);

							//ITEMに図書情報を設定
							std::string binf = mysqli::mysqli_str_to_string(book_info["Title"]) + " / "
													+ mysqli::mysqli_str_to_string(book_info["Author"]);
							scr->get_object("ITEM" + ofs_tmp.str())->set_attr("text", binf);
						}catch(std::string message){
							//エラーなら要素を削除しループを戻る
							isbn_list_tmp.erase(isbn_list_tmp.begin() + list_ptr);
							--idx;
						}
					}else{
						//リスト要素がアイテムに対して少ないときはアイテムをクリア
						scr->get_object("ITEM" + ofs_tmp.str())->set_attr("text", std::string(""));
					}
				}

				//リストの変更を完了する
				pub_vars["book_list"] = isbn_list_tmp;
				pub_vars["old_book_list_num"] = (int)(isbn_list_tmp.size());
				pub_vars["book_list_old_page"] = cur_page;
				pub_vars["book_list_reset"] = 0;


				//現在のページ番号を反映
				std::ostringstream page_str;

				page_str << std::setfill('0') << std::setw(2) << (cur_page + 1);
				page_str  << " / ";
				page_str << std::setfill('0') << std::setw(2) << (int)(isbn_list_tmp.size() == 0 ? 1 : ROUND_UP(isbn_list_tmp.size() / 5.0));
				scr->get_object("PAGE_NUM")->set_attr("text", page_str.str());
			}

			//ISBNリストが０でID:ENTERが表示状態なら
			scr_object::ScrObjectPtr ent_obj = scr->get_object("ENTER");
			if(isbn_list_tmp.size() == 0 && ent_obj->get_visible()){
				//非表示にする
				ent_obj->set_visible(0);
			}else if(isbn_list_tmp.size() > 0 && !ent_obj->get_visible()){
				//ISBNリストが１以上でID:ENTERが非表示状態なら
				//表示させる
				ent_obj->set_visible(1);
			}
		});

		//<<のボタンが押された時の動作
		scr->get_object("PREV_PAGE")->set_pushup_extra([screens](scr_object* obj){
			int cur_page = boost::any_cast<int>(pub_vars["book_list_cur_page"]);

			if(cur_page - 1 >= 0){
				pub_vars["book_list_cur_page"] = cur_page - 1;
			}
		});
		//>>のボタンが押された時の動作
		scr->get_object("NEXT_PAGE")->set_pushup_extra([screens](scr_object* obj){
			int cur_page = boost::any_cast<int>(pub_vars["book_list_cur_page"]);

			//ISBNリスト取得
			std::vector<std::string> isbn_list_tmp = boost::any_cast<std::vector<std::string> >(pub_vars["book_list"]);
			if(cur_page + 1 < ROUND_UP(isbn_list_tmp.size() / 5.0)){
				pub_vars["book_list_cur_page"] = cur_page + 1;
			}
		});

		//EXPLAINがタッチされた時の動作
		for(int idx = 1; 1; idx++){
			std::ostringstream ofs_tmp;

			ofs_tmp << idx;

			scr_object::ScrObjectPtr obj = scr->get_object("EXPLAIN" + ofs_tmp.str());

			if(obj->get_id() == "") break; //ダミーになったら終了

			obj->set_pushup_extra([](scr_object* obj){
				screen* scr = obj->get_parent();

				//表示状態設定
				for(int idx = 1; idx <= 5; idx++){
					std::ostringstream ofs_tmp;

					ofs_tmp << idx;

					//EXPLAIN設定
					scr->get_object("EXPLAIN" + ofs_tmp.str())->set_visible(0);
					//ITEM設定
					scr->get_object("ITEM" + ofs_tmp.str())->set_visible(1);
					//IDEL設定
					scr->get_object("IDEL" + ofs_tmp.str())->set_visible(1);
					//INFO設定
					scr->get_object("INFO" + ofs_tmp.str())->set_visible(1);
				}
			});
		}

		//IDEL,INFOの動作
		for(int idx = 1; idx <= 5; idx++){
			std::ostringstream ofs_tmp;
			ofs_tmp << idx;

			//IDEL設定
			scr->get_object("IDEL" + ofs_tmp.str())->set_pushup_extra([idx](scr_object* obj){
				//ISBNリスト取得
				std::vector<std::string> isbn_list_tmp = boost::any_cast<std::vector<std::string> >(pub_vars["book_list"]);

				int cur_page = boost::any_cast<int>(pub_vars["book_list_cur_page"]);

				//インデックス計算
				int list_ptr = cur_page * 5 + (idx - 1);
				if(list_ptr < isbn_list_tmp.size()){
					//削除
					isbn_list_tmp.erase(isbn_list_tmp.begin() + list_ptr);

					pub_vars["book_list"] = isbn_list_tmp;
				}

				//現在のページ数を最後に設定
				pub_vars["book_list_cur_page"] = (isbn_list_tmp.size() == 0 ? 0 : (ROUND_UP(isbn_list_tmp.size() / 5.0) - 1));
			});

			//INFO設定
			scr->get_object("INFO" + ofs_tmp.str())->set_pushup_extra([idx](scr_object* obj){
				//ISBNリスト取得
				std::vector<std::string> isbn_list_tmp = boost::any_cast<std::vector<std::string> >(pub_vars["book_list"]);

				int cur_page = boost::any_cast<int>(pub_vars["book_list_cur_page"]);

				//インデックス計算
				int list_ptr = cur_page * 5 + (idx - 1);
				if(list_ptr < isbn_list_tmp.size()){
					std::string isbn = isbn_list_tmp[list_ptr];

					//図書情報を取得
					book_manager::BOOK_INFO book_info = book_man->search_book(isbn);

					//詳細表示
					std::ostringstream binfo;

					//DATEは日付部分のみ取得
					std::string rem_date = mysqli::mysqli_str_to_string(book_info["Date"]);
					rem_date = rem_date.substr(0, rem_date.find(' '));

					binfo << "・図書情報\n";
					binfo << "タイトル: " << mysqli::mysqli_str_to_string(book_info["Title"]) << "\n";
					binfo << "著者: " << mysqli::mysqli_str_to_string(book_info["Author"]) << "\n";
					binfo << "出版社: " << mysqli::mysqli_str_to_string(book_info["Publisher"]) << "\n";
					binfo << "発売日: " << rem_date << "\n";
					binfo << "ページ数: " << mysqli::mysqli_str_to_string(book_info["NumberOfPages"]) << "\n";
					binfo << "価格: " << mysqli::mysqli_str_to_string(book_info["Price"]);

					pub_funcs["ShowBookInfo"](binfo.str());

					if(book_info["image"].size() > 0){
						std::vector<unsigned char> img_data_tmp;
						std::copy(book_info["image"].begin(), book_info["image"].end(), std::inserter(img_data_tmp, img_data_tmp.begin()));

						obj->get_parent()->get_object("COVER_IMG")->set_attr("data", img_data_tmp);
					}
				}
			});
		}

		//ID:ENTERの動作
		scr->get_object("ENTER")->set_pushup_extra([screens](scr_object* obj){
			std::string scr_id = obj->get_parent()->get_id();

			//ISBNリスト取得
			std::vector<std::string> isbn_list_tmp = boost::any_cast<std::vector<std::string> >(pub_vars["book_list"]);

			//ENTERはBORROW_SCR,RETURN_SCRのみで有効
			if(scr_id == "BORROW_SCR"){
				//借りる本をDBに追加
				book_man->borrow_books(login_con->get_login_user(), isbn_list_tmp);

				pub_funcs["MessageBoxGoTop"](std::string("図書借用処理が完了しました"));
			}else if(scr_id == "RETURN_SCR"){
				//返す本をDBに追加
				book_man->return_books(isbn_list_tmp);

				pub_funcs["MessageBoxGoTop"](std::string("図書返却処理が完了しました"));
			}
		});


		//ID:CANCELの動作
		scr->get_object("CANCEL")->set_pushup_extra([screens](scr_object* obj){
			if(login_con->is_admin()){
				screens->select("TOP_ADMIN");
			}else{
				screens->select("TOP_NORMAL");
			}
		});

		//アイテム欄タッチ動作設定
		for(int idx = 1; idx <= 5; idx++){
			std::ostringstream ofs_tmp;

			ofs_tmp << idx;

			std::string scr_id = scr->get_id();
			scr_object::ScrObjectPtr obj = scr->get_object("ITEM" + ofs_tmp.str());

			obj->set_pushup_extra([screens, scr_id](scr_object* obj){
				//戻り先スクリーンID格納
				pub_vars["keypad_return_id"] = scr_id;

				//文字設定用オブジェクトセット
				pub_vars["keypad_setstr_object"] = obj;

				//キーボードへ遷移
				screens->select("KEYBOARD_PAD");
			});
		}
	}

	//---フレームバッファ関連のイベント
	scr_ptr = screens->get_screen("FRAMEBUFFER");

	scr_ptr->set_load_extra([](screen* scr){
		//ログインIDセット
		scr->get_object("L_FORM_STATUS")->set_attr("text", login_con->get_login_user());

		//キーパッド戻りなら実行する
		if(boost::any_cast<std::string>(pub_vars["keypad_return_id"]) == scr->get_id()){
			//コマンド文字列取得
			boost::any* cmd_text_ptr = scr->get_object("COMMAND_TEXT")->get_attr("text");

			if(cmd_text_ptr){
				std::string cmd_text = boost::any_cast<std::string>(*cmd_text_ptr);

				// コンソールにコマンド送信
				int fb_fd = boost::any_cast<int>(pub_vars["fb_fd"]);

				if(fb_fd >= 0){

					char buff[2];
					buff[1] = '\0';
					for(int idx = 0; idx < cmd_text.length(); idx++){
						char str_char = cmd_text[idx];

						//エスケープ文字の場合
						if(str_char == '^'){
							if(idx + 1 < cmd_text.length()){
								idx++;

								str_char = cmd_text[idx];

								//小文字の場合は大文字に変換
								if(str_char >= 'a' && str_char <= 'z'){
									str_char = str_char % 'a' + 'A';
								}

								//エスケープ文字の範囲に入っていれば
								if(str_char >= 'A' && str_char <= '_'){
									//制御文字に置き換える
									str_char = str_char % 'A' + 1;
								}else{
									//エスケープ文字に入っていない場合は前の文字に戻る
									idx--;
									str_char = cmd_text[idx];
								}
							}
						}
						buff[0] = str_char;

						ioctl(fb_fd, TIOCSTI, buff);
					}
				}
			}
		}
	});

	scr_ptr->set_interval_extra([](screen* scr){
		scr->get_object("FB")->onPushUpDraw();
		usleep(100000);
	});

	scr_ptr->get_object("RETURN_BUTTON")->set_pushup_extra([screens](scr_object* obj){
		std::string scr_id = obj->get_parent()->get_id();

		//トップ画面へ戻る
		if(login_con->is_admin()){
			screens->select("TOP_ADMIN");
		}else{
			screens->select("TOP_NORMAL");
		}
	});

	scr_ptr->get_object("FB")->set_pushup_extra([screens](scr_object* obj){
		std::string scr_id = obj->get_parent()->get_id();

		//戻り先スクリーンID格納
		pub_vars["keypad_return_id"] = scr_id;

		//文字設定用オブジェクトセット
		pub_vars["keypad_setstr_object"] = obj->get_parent()->get_object("COMMAND_TEXT").get();

		//キーボードへ遷移
		screens->select("KEYBOARD_LOWER");
	});




	//---キーパッド関連のイベント

	//入力のイベント登録
	std::vector<std::string> pad_scr_id_list = {
		"KEYBOARD_PAD", "KEYBOARD_LOWER", "KEYBOARD_UPPER", "KEYBOARD_SYMBOL"
	};
	std::vector< std::vector<std::string> > padobj_id_list = {
		{"KEY1", "KEY2", "KEY3", "KEY4", "KEY5", "KEY6", "KEY7", "KEY8", "KEY9", "KEY0", "KEYX"}, //KEYBOARD_PAD
		{"KEY1", "KEY2", "KEY3", "KEY4", "KEY5", "KEY6", "KEY7", "KEY8", "KEY9", "KEY0",
		"KEYQ", "KEYW", "KEYE", "KEYR", "KEYT", "KEYY", "KEYU", "KEYI", "KEYO", "KEYP",
		"KEYA", "KEYS", "KEYD", "KEYF", "KEYG", "KEYH", "KEYJ", "KEYK", "KEYL", "KEYZ",
		"KEYX", "KEYC", "KEYV", "KEYB", "KEYN", "KEYM", "RETURN"}, //KEYBOARD_LOWER
		{"KEY1", "KEY2", "KEY3", "KEY4", "KEY5", "KEY6", "KEY7", "KEY8", "KEY9", "KEY0",
		"KEYQ", "KEYW", "KEYE", "KEYR", "KEYT", "KEYY", "KEYU", "KEYI", "KEYO", "KEYP",
		"KEYA", "KEYS", "KEYD", "KEYF", "KEYG", "KEYH", "KEYJ", "KEYK", "KEYL", "KEYZ",
		"KEYX", "KEYC", "KEYV", "KEYB", "KEYN", "KEYM", "RETURN"}, //KEYBOARD_UPPER
		{"KEYS1", "KEYS2", "KEYS3", "KEYS4", "KEYS5", "KEYS6", "KEYS7", "KEYS8", "KEYS9", "KEYS10",
		"KEYS11", "KEYS12", "KEYS13", "KEYS14", "KEYS15", "KEYS16", "KEYS17", "KEYS18", "KEYS19", "KEYS20",
		"KEYS21", "KEYS22", "KEYS23", "KEYS24", "KEYS25", "KEYS26", "KEYS27", "KEYS28", "KEYS29", "KEYS30",
		"KEYS31", "KEYS32", "SPACE", "TAB", "RETURN"}, //KEYBOARD_SYMBOL
	};

	for(int scr_idx = 0; scr_idx < pad_scr_id_list.size(); scr_idx++){
		std::string scr_id = pad_scr_id_list[scr_idx];

		//バーコードリーダ用イベント登録
		screens->get_screen(scr_id)->set_interval_extra([](screen* scr){
			std::string barcode = barcode_reader.get_barcode();

			//読み取られたバーコードがあれば
			if(barcode != ""){
				//このスクリーンのINSTRINGに設定し確定を呼び出す
				scr->get_object("INSTRING")->set_attr("text", barcode);
				scr->get_object("ENTER")->call_pushup_extra();
			}
		});

		//キー入力関連のイベント登録
		for(int obj_idx = 0; obj_idx < padobj_id_list[scr_idx].size(); obj_idx++){
			std::string obj_id = padobj_id_list[scr_idx][obj_idx];

			screens->get_screen(scr_id)->get_object(obj_id)->set_pushup_extra([](scr_object* obj){
				boost::any* obj_text = obj->get_attr("text");
				boost::any* istr_val = obj->get_parent()->get_object("INSTRING")->get_attr("text");

				if(obj_text && istr_val){
					std::string str_tmp = boost::any_cast<std::string>(*istr_val);

					//オブジェクトIDがSPACEならホワイトスペース挿入
					if(obj->get_id() == "SPACE"){
						str_tmp += " ";
					}else if(obj->get_id() == "TAB"){
						//タブならタブ文字挿入
						str_tmp += "<\\t>";
					}else if(obj->get_id() == "RETURN"){
						//RETURNなら改行コード挿入
						str_tmp += "<\\n>";
					}else{
						str_tmp += boost::any_cast<std::string>(*obj_text);
					}

					obj->get_parent()->get_object("INSTRING")->set_attr("text", str_tmp);
				}
			});
		}

		//キー削除関連のイベント登録
		screens->get_screen(scr_id)->get_object("DELETE")->set_pushdown_extra([](scr_object* obj){
			std::string before_string = "";
			if(boost::any* instr_tmp = obj->get_parent()->get_object("INSTRING")->get_attr("text")){
				before_string = boost::any_cast<std::string>(*instr_tmp);
			}

			int cut_size = before_string.length() - 1;
			if(cut_size < 0) cut_size = 0;

			obj->get_parent()->get_object("INSTRING")->set_attr("text", before_string.substr(0, cut_size));
		});

		//キーパッド変更関連の処理
		screens->get_screen(scr_id)->get_object("LOWER")->set_pushup_extra([screens](scr_object* obj){

			std::string before_string = "";
			if(boost::any* instr_tmp = screens->cur()->get_object("INSTRING")->get_attr("text")){
				before_string = boost::any_cast<std::string>(*instr_tmp);
			}

			screens->select("KEYBOARD_LOWER");

			//変更先のキーパッドに前の状態のテキスト設定
			screens->cur()->get_object("INSTRING")->set_attr("text", before_string);
		});
		screens->get_screen(scr_id)->get_object("UPPER")->set_pushup_extra([screens](scr_object* obj){

			std::string before_string = "";
			if(boost::any* instr_tmp = screens->cur()->get_object("INSTRING")->get_attr("text")){
				before_string = boost::any_cast<std::string>(*instr_tmp);
			}

			screens->select("KEYBOARD_UPPER");

			//変更先のキーパッドに前の状態のテキスト設定
			screens->cur()->get_object("INSTRING")->set_attr("text", before_string);
		});
		screens->get_screen(scr_id)->get_object("SYMBOL")->set_pushup_extra([screens](scr_object* obj){

			std::string before_string = "";
			if(boost::any* instr_tmp = screens->cur()->get_object("INSTRING")->get_attr("text")){
				before_string = boost::any_cast<std::string>(*instr_tmp);
			}

			screens->select("KEYBOARD_SYMBOL");

			//変更先のキーパッドに前の状態のテキスト設定
			screens->cur()->get_object("INSTRING")->set_attr("text", before_string);
		});


		//キーパッド確定のイベント登録
		screens->get_screen(scr_id)->get_object("ENTER")->set_pushup_extra([screens, pad_scr_id_list](scr_object* obj){
			if(boost::any* instr_tmp = screens->cur()->get_object("INSTRING")->get_attr("text")){
				std::string str_tmp = boost::any_cast<std::string>(*instr_tmp);
				scr_object* obj_tmp = boost::any_cast<scr_object*>(pub_vars["keypad_setstr_object"]);

				//返却先オブジェクトにテキスト設定
				if(obj_tmp != NULL){
					//改行とタブ文字に置き換える
					int pos_tmp = 0;
					while((pos_tmp = str_tmp.find("<\\t>", pos_tmp)) != std::string::npos){
						str_tmp.replace(pos_tmp, 4, "\t");
					}
					pos_tmp = 0;
					while((pos_tmp = str_tmp.find("<\\n>", pos_tmp)) != std::string::npos){
						str_tmp.replace(pos_tmp, 4, "\n");
					}

					obj_tmp->set_attr("text", str_tmp);
				}
			}

			//文字入力部クリア
			for(int scr_id = 0; scr_id < pad_scr_id_list.size(); scr_id++){
				screens->get_screen(pad_scr_id_list[scr_id])->get_object("INSTRING")->set_attr("text", std::string(""));
			}

			//前の状態へ戻る
			screens->select(boost::any_cast<std::string>(pub_vars["keypad_return_id"]));
		});
	}
}
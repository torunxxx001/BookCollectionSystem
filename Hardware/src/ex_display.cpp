#include "lib.hpp"

#include "ex_display.hpp"
#include "ex_scr_obj.hpp"
#include "fb_scr_obj.hpp"

#include "disp_lib.hpp"
#include "dstring.hpp"

#include "xml_lib.hpp"

#include "events.hpp"

#include <iostream>
#include <vector>
#include <sstream>
#include <functional>
#include <sstream>
#include <string.h>
#include <time.h>

#include <boost/any.hpp>

ex_display::ex_display(std::string screen_list_file, std::string touch_setting_file)
{
	this->touch_status = 0;

	//タッチスクリーン情報読み込み・保存場所設定
	this->touch_setting_file_path = touch_setting_file;
	memset(this->touch_setting, 0, sizeof(this->touch_setting));

	this->old_screen_id = "";

	//スクリーンオブジェクト定義ファイル読み取り処理
	c_xml scr_setting(screen_list_file);
	c_xml::TAG_LIST_PTR root = scr_setting.get_root();

	//ルートタグの名前を比較
	if(root->size() == 0 || root->at(0).tag_name != "display"){
		throw std::string("error: スクリーン定義ファイルのフォーマットが不正です");
	}

	//fps属性がセットされていたら反映
	TAG_ATTR_LIST::iterator attr_it = root->at(0).attributes.find("fps");
	if(attr_it != root->at(0).attributes.end()){
		//FPSをセット
		std::istringstream is_fps((*attr_it).second);

		double fps_tmp;
		if(is_fps >> fps_tmp){
			this->setFPS(fps_tmp);
		}
	}

	//sleep関連値リセット
	this->captured_time = 0;
	this->sleep_wait_time = 0;

	//sleep属性がセットされていたら反映
	attr_it = root->at(0).attributes.find("sleep");
	if(attr_it != root->at(0).attributes.end()){
		//sleep値をセット
		std::istringstream is_sleep((*attr_it).second);

		int sleep_sec;
		if(is_sleep >> sleep_sec){
			this->sleep_wait_time = sleep_sec;
		}
	}

	std::string default_screen_id = ""; //デフォルト選択されたスクリーンのID
	//screenタグのループ
	for(int scr_index = 0; scr_index < root->at(0).child_elements.size(); scr_index++){
		TAG_ELEMENT* tag_ptr = &(root->at(0).child_elements[scr_index]);

		//タグ名がスクリーンかチェック
		if(tag_ptr->tag_name != "screen") continue;

		std::string scr_id = tag_ptr->attributes["id"];
		std::string str_color = tag_ptr->attributes["color"];
		std::string str_direction = tag_ptr->attributes["direction"];

		int color = gdTrueColor(255, 255, 255);
		int direction = DIRECTION_NORMAL;

		//colorとdirectionを数値に変換
		std::istringstream is_color(str_color);

		//正しい色情報かチェックし取得
		unsigned char prefix;
		if((is_color >> prefix) && prefix == '#'){
			is_color >> std::hex >> color;
		}

		//画面方向を取得し設定
		std::transform(str_direction.begin(), str_direction.end(),
				str_direction.begin(), ::tolower);

		if(str_direction == "up") direction = DIRECTION_UP;
			else if(str_direction == "right") direction = DIRECTION_RIGHT;
			else if(str_direction == "left") direction = DIRECTION_LEFT;

		try {
			//タグ情報を元にスクリーンを追加
			this->screens.add(screen::create(scr_id));
			this->screens.select(scr_id);

			//スクリーン設定をセット
			this->screens.cur()
					->set_color(color)
					->set_direction(direction);


			//一番最初のスクリーン要素IDを一応格納
			if(default_screen_id == ""){
				default_screen_id = scr_id;
			}
			//default属性がある場合はそのスクリーンIDを格納
			TAG_ATTR_LIST::iterator f_it = tag_ptr->attributes.find("default");
			if(f_it != tag_ptr->attributes.end()){
				default_screen_id = scr_id;
			}

			//スクリーン内オブジェクトの情報をセット
			for(int obj_index = 0; obj_index < tag_ptr->child_elements.size(); obj_index++){
				//オブジェクトのエラー処理
				try {

					TAG_ELEMENT* c_tag_ptr = &(tag_ptr->child_elements.at(obj_index));

					//オブジェクト基本情報取得
					std::string obj_id = c_tag_ptr->attributes["id"];
					std::string str_position = c_tag_ptr->attributes["position"];
					std::string str_size = c_tag_ptr->attributes["size"];

					POINT position = {};
					POINT size = {};

					//座標を取得
					std::istringstream is_pos(str_position);
					std::istringstream is_size(str_size);

					unsigned char pos_sep, size_sep;

					is_pos >> position.x >> pos_sep >> position.y;
					is_size >> size.x >> size_sep >> size.y;

					//座標が正しく取得できなかったら飛ばす
					if(pos_sep != ',' || size_sep != ','){
						continue;
					}

					scr_object::ScrObjectPtr obj_ptr = NULL;
					if(c_tag_ptr->tag_name == "button"){
						obj_ptr = button::create(obj_id, position.x, position.y, size.x, size.y);

						//ボタンのみタッチ有効化
						obj_ptr->set_enable(1);
					}else if(c_tag_ptr->tag_name == "text"){
						obj_ptr = text::create(obj_id, position.x, position.y, size.x, size.y);
					}else if(c_tag_ptr->tag_name == "password"){
						obj_ptr = password::create(obj_id, position.x, position.y, size.x, size.y);
					}else if(c_tag_ptr->tag_name == "image"){
						obj_ptr = image::create(obj_id, position.x, position.y, size.x, size.y);
					}else if(c_tag_ptr->tag_name == "framebuffer"){
						obj_ptr = fb_object::create(obj_id, position.x, position.y, size.x, size.y);
					}

					//オブジェクトの設定＆追加
					if(obj_ptr != NULL){
						//表示有効化
						obj_ptr->set_visible(1);

						//memo: set_attrについて
						//そのオブジェクトに有効なnameのみ受け入れられる
						//またnameと合致した型の値のみ受け入れられる

						//タグ内の値をtextとして追加を試みる
						obj_ptr->set_attr("text", c_tag_ptr->value);

						//タグの属性分ループ
						TAG_ATTR_LIST::iterator at_itr = c_tag_ptr->attributes.begin();
						for(; at_itr != c_tag_ptr->attributes.end(); at_itr++){
							std::string name = (*at_itr).first;
							std::string value = (*at_itr).second;

							//属性値は小文字にしておく
							std::transform(value.begin(), value.end(),
													value.begin(), ::tolower);

							//オブジェクト属性の型取得
							const std::type_info* type_name = obj_ptr->get_attr_type(name);

							if(type_name != NULL){
								//属性の型名により処理を分岐
								if(*type_name == typeid(std::string)){
									//nameがpathならフルパス追加
									if(name == "path"){
										value = lib::get_local_path() + value;
									}

									//文字として追加
									obj_ptr->set_attr(name, value);
								}else if(*type_name == typeid(int)){
									//int値

									std::istringstream is_val(value);

									//色情報かチェック
									unsigned char prefix;
									if((is_val >> prefix) && prefix == '#'){
										int color_tmp;

										if(is_val >> std::hex >> color_tmp){
											//色情報として情報追加
											obj_ptr->set_attr(name, color_tmp);
										}
									}else{
										//違ったらただの数値として追加
										is_val.str(value);
										is_val.clear();

										int int_tmp;
										if(is_val >> int_tmp){
											obj_ptr->set_attr(name, int_tmp);
										}
									}
								}else if(*type_name == typeid(double)){
									//浮動小数点数
									std::istringstream is_val(value);
									double double_tmp;

									if(is_val >> double_tmp){
										obj_ptr->set_attr(name, double_tmp);
									}
								}else if(*type_name == typeid(STR_ALIGN)){
									//文字並び情報

									STR_ALIGN align_info;

									if(name == "valign"){
										align_info = VTOP;

										//垂直並び情報
										if(value == "center"){
											align_info = VCENTER;
										}else if(value == "bottom"){
											align_info = VBOTTOM;
										}
									}else if(name == "halign"){
										align_info = HLEFT;

										//水平並び情報
										if(value == "center"){
											align_info = HCENTER;
										}else if(value == "right"){
											align_info = HRIGHT;
										}
									}

									obj_ptr->set_attr(name, align_info);
								}else if(*type_name == typeid(FB_DEGREE)){
									//フレームバッファの向き
									if(value == "0"){
										obj_ptr->set_attr(name, R_0);
									}else if(value == "1"){
										obj_ptr->set_attr(name, R_90);
									}else if(value == "2"){
										obj_ptr->set_attr(name, R_180);
									}else if(value == "3"){
										obj_ptr->set_attr(name, R_270);
									}
								}
							}else{
								//オブジェクトに存在しない属性でvisible,enableは別処理

								if(name == "visible"){
									if(value == "0"){
										obj_ptr->set_visible(0);
									}else{
										obj_ptr->set_visible(1);
									}
								}else if(name == "enable"){
									if(value == "0"){
										obj_ptr->set_enable(0);
									}else{
										obj_ptr->set_enable(1);
									}
								}
							}
						}

						this->screens.cur()->add(obj_ptr);
					}
				} catch(std::string message) {
					std::cerr << "warning: " << message << std::endl;
				}
			}
		} catch(std::string message) {
			std::cerr << "warning: " << message << std::endl;
		}
	}


	//イベント情報登録(events.h)
	registEvents(&screens);

	//screen_containerのcreateを呼び出し
	this->screens.call_create_extra();

	//デフォルトスクリーンを登録、同時にdefault_screen_idのloadが呼び出される
	this->screens.select(default_screen_id);


	//sleep用タイムキャプチャ
	this->captured_time = time(NULL);
}

ex_display::~ex_display()
{
	this->exit();
}

void ex_display::start_touch_adjust()
{
	dstring d_str(16);

	int white = gdTrueColor(255, 255, 255);
	int black = gdTrueColor(0, 0, 0);

	gdImageFilledRectangle(this->get_image(), 0, 0, TFT_WIDTH, TFT_HEIGHT, white);

	d_str.draw_string(this->get_image(), (TFT_WIDTH - 180) / 2, TFT_HEIGHT/2 - 80, 180, 40, HCENTER, VCENTER, black, white, "タッチパネル補正");
	d_str.draw_string(this->get_image(), (TFT_WIDTH - 180) / 2, TFT_HEIGHT/2, 180, 40, HCENTER, VCENTER, black, white, "1番をタッチ");

	gdImageFilledArc(this->get_image(), 10, 10, 10, 10, 0, 360, black, black);
	d_str.draw_string(this->get_image(), 30, 5, 20, 20, HCENTER, VCENTER, black, white, "3");


	gdImageFilledArc(this->get_image(), TFT_WIDTH-10, 10, 10, 10, 0, 360, black, black);
	d_str.draw_string(this->get_image(), TFT_WIDTH-40, 5, 20, 20, HCENTER, VCENTER, black, white, "4");

	gdImageFilledArc(this->get_image(), TFT_WIDTH-10, TFT_HEIGHT-10, 10, 10, 0, 360, black, black);
	d_str.draw_string(this->get_image(), TFT_WIDTH-40, TFT_HEIGHT-25, 20, 20, HCENTER, VCENTER, black, white, "1");

	gdImageFilledArc(this->get_image(), 10, TFT_HEIGHT-10, 10, 10, 0, 360, black, black);
	d_str.draw_string(this->get_image(), 30, TFT_HEIGHT-25, 20, 20, HCENTER, VCENTER, black, white, "2");

	memset(this->touch_setting, 0, sizeof(this->touch_setting));

	POINT max_pos = {0};
	int process = 0;
	while(1){
		if(process < 4){
			POINT pos;
			this->board_control->get_touch_pos(&pos);

			if(pos.x >= 20 && pos.y >= 20){
				if(max_pos.x < pos.x) max_pos.x = pos.x;
				if(max_pos.y < pos.y) max_pos.y = pos.y;

				if(pos.x < max_pos.x/2 && pos.y < max_pos.y/2){
					if(process == 2){
						memcpy(&this->touch_setting[0], &pos, sizeof(POINT));
						d_str.draw_string(this->get_image(), (TFT_WIDTH - 180) / 2, TFT_HEIGHT/2, 180, 40, HCENTER, VCENTER, black, white, "4番をタッチ");
						process = 3;
					}
				}else if(pos.x > max_pos.x/2 && pos.y < max_pos.y/2){
					if(process == 3){
						memcpy(&this->touch_setting[1], &pos, sizeof(POINT));
						d_str.draw_string(this->get_image(), (TFT_WIDTH - 180) / 2, TFT_HEIGHT/2, 180, 40, HCENTER, VCENTER, black, white, "設定完了");
						process = 4;
					}
				}else if(pos.x > max_pos.x/2 && pos.y > max_pos.y/2){
					memcpy(&this->touch_setting[2], &pos, sizeof(POINT));
					d_str.draw_string(this->get_image(), (TFT_WIDTH - 180) / 2, TFT_HEIGHT/2, 180, 40, HCENTER, VCENTER, black, white, "2番をタッチ");
					process = 1;
				}else{
					if(process == 1){
						memcpy(&this->touch_setting[3], &pos, sizeof(POINT));
						d_str.draw_string(this->get_image(), (TFT_WIDTH - 180) / 2, TFT_HEIGHT/2, 180, 40, HCENTER, VCENTER, black, white, "3番をタッチ");
						process = 2;
					}
				}
			}
		}else{
			//幅・高さの電圧量を求める
			int vol_width = ABS(this->touch_setting[0].x - this->touch_setting[1].x);
			int vol_height = ABS(this->touch_setting[0].y - this->touch_setting[3].y);

			//１ピクセルあたりの電圧数値を求める
			double vol_pix_x = (double)vol_width / (TFT_WIDTH - 20);
			double vol_pix_y = (double)vol_height / (TFT_HEIGHT - 20);

			//基準点のマージンを削除
			this->touch_setting[0].x -= vol_pix_x * 10;
			this->touch_setting[0].y -= vol_pix_y * 10;

			this->touch_setting[1].x += vol_pix_x * 10;
			this->touch_setting[1].y -= vol_pix_y * 10;

			this->touch_setting[2].x += vol_pix_x * 10;
			this->touch_setting[2].y += vol_pix_y * 10;

			this->touch_setting[3].x -= vol_pix_x * 10;
			this->touch_setting[3].y += vol_pix_y * 10;

			break;
		}

		display::onDraw();

		usleep(100000);
	}

	//設定が決まったら保存
	std::ofstream s_file(this->touch_setting_file_path);
	if( !s_file.fail()){
		for(int i = 0; i < 4; i++){
			s_file << this->touch_setting[i].x << ","
				<< this->touch_setting[i].y << std::endl;
		}
	}
}

void ex_display::load_touch_setting()
{
	std::ifstream in_file(this->touch_setting_file_path);

	int load_ok = 0;
	if( !in_file.fail()){
		std::string sbuffer;

		std::vector<POINT> points;
		while(in_file >> sbuffer){
			std::istringstream str_p(sbuffer);

			POINT p_tmp;
			char delim;

			str_p >> p_tmp.x >> delim >> p_tmp.y;

			//区切り文字がカンマならば
			if(delim == ','){
				points.push_back(p_tmp);
			}
		}

		if(points.size() == 4){
			for(int i = 0; i < points.size(); i++){
				this->touch_setting[i].x = points[i].x;
				this->touch_setting[i].y = points[i].y;
			}
			load_ok = 1;
		}
	}

	if(load_ok == 0){
		this->start_touch_adjust();
	}
}

int ex_display::get_touch_xy(POINT* put_pos)
{
	std::function<void (POINT* ret_pos)> sub_get_xy;

	sub_get_xy = [&](POINT* ret_pos){
		POINT vol_pos;
		this->board_control->get_touch_pos(&vol_pos);

		//vol_posがある閾値以下なら
		if(vol_pos.x <= 20 || vol_pos.y <= 20){
			ret_pos->x = -1;
			ret_pos->y = -1;
			return;
		}

		//上端・下端での幅と、左端・右端での高さを求める
		int x_sub1 = ABS(this->touch_setting[0].x - this->touch_setting[1].x);
		int x_sub2 = ABS(this->touch_setting[2].x - this->touch_setting[3].x);
		int y_sub1 = ABS(this->touch_setting[0].y - this->touch_setting[3].y);
		int y_sub2 = ABS(this->touch_setting[1].y - this->touch_setting[2].y);

		//上端・下端での幅の差と、左端・右端での高さの差を求める
		int diff_x = x_sub2 - x_sub1;
		int diff_y = y_sub2 - y_sub1;

		//電圧量での幅・高さを、上端・左端のものとする
		int vol_width = x_sub1;
		int vol_height = y_sub1;

		//pos.xとpos.yが0から始まるようにする
		vol_pos.x -= this->touch_setting[0].x;
		vol_pos.y -= this->touch_setting[0].y;

		//割合を取る
		double ratio_x = (double)vol_pos.x / vol_width;
		double ratio_y = (double)vol_pos.y / vol_height;
		//xとyの割合を掛ける
		double mul_xy = ratio_x * ratio_y;

		//xとyのピクセルを求める(その際微調整)
		int x = (int)((double)(vol_pos.x - diff_x * mul_xy) / vol_width * TFT_WIDTH);
		int y = (int)((double)(vol_pos.y - diff_y * mul_xy) / vol_height * TFT_HEIGHT);
		if(x < 0) x = 0;
		if(x > TFT_WIDTH) x = TFT_WIDTH;
		if(y < 0) y = 0;
		if(y > TFT_HEIGHT) y = TFT_HEIGHT;

		ret_pos->x = x;
		ret_pos->y = y;
	};

	//出力を-1で初期化
	put_pos->x = put_pos->y = -1;

	//タッチ部分の精度を上げる処理
	POINT tmpPos;

	sub_get_xy(&tmpPos);

	//タッチ状態なら実行
	if(tmpPos.x != -1 && tmpPos.y != -1){
		double sum_x_diff = 0;
		double sum_y_diff = 0;
		for(int i = 0; i < 16; i++){
			POINT stPos;
			sub_get_xy(&stPos);

			sum_x_diff += ABS(stPos.x - tmpPos.x);
			sum_y_diff += ABS(stPos.y - tmpPos.y);

			usleep(10);
		}
		sum_x_diff /= 16;
		sum_y_diff /= 16;

		if(sum_x_diff < 2 && sum_y_diff < 2){
			memcpy(put_pos, &tmpPos, sizeof(POINT));
		}
	}

	if(put_pos->x == -1 || put_pos->y == -1){
		return 0;
	}else{
		return 1;
	}
}

void ex_display::onInit()
{
	this->load_touch_setting();
}

void ex_display::onDraw()
{
	//タッチパネル検出
	POINT tPos;
	if(this->get_touch_xy(&tPos)){
		//ディスプレイがON状態でタッチを受け付ける
		if(this->get_display_status() == DISPLAY_STATUS_ON){
			//座標変換
			disp_lib::calcCoordReverse(this->screens.cur()->get_direction(), &tPos, 1);

			//タッチされていたら
			this->screens.cur()->send_pushdown(tPos.x, tPos.y);
		}


		this->touch_status = 1;
	}else{
		if(this->touch_status == 1){
			this->screens.cur()->send_pushup();


			//sleepリセット
			if(this->sleep_wait_time != 0){
				this->captured_time = time(NULL);

				//displayがオフ状態なら復帰
				if(this->get_display_status() == DISPLAY_STATUS_OFF){
					this->display_on();
				}
			}


			this->touch_status = 0;
		}
	}

	//displayオン状態で経過時間が過ぎたらdisplayオフ
	if(this->get_display_status() == DISPLAY_STATUS_ON){
		if(this->sleep_wait_time != 0 &&
			this->sleep_wait_time < time(NULL) - this->captured_time)
		{
			this->display_off();
		}
	}



	//描画に関する処理を書く
	int update_tft = 0; //TFTへ画素を転送するかのフラグ
	int all_redraw = 0;

	if(this->screens.cur()->get_id() != this->old_screen_id){
		this->old_screen_id = this->screens.cur()->get_id();

		//スクリーンが切り替わったら
		all_redraw = 1;
	}
	std::vector<scr_object::ScrObjectPtr> need_draw_objects;

	need_draw_objects = this->screens.cur()->get_need_draw();
	for(int i = 0; i < need_draw_objects.size(); i++){
		//オブジェクト描画フラグがDRAW_CLEARかチェック
		if(need_draw_objects[i]->get_need_draw() == DRAW_CLEAR){
			//一致したらオブジェクト全再描画
			all_redraw = 1;
			break;
		}
	}

	if(all_redraw){
		//全描画の処理

		//画面をスクリーン色で塗りつぶす
		gdImageFilledRectangle(this->get_image(), 0, 0, TFT_WIDTH, TFT_HEIGHT, this->screens.cur()->get_color());

		//各種オブジェクトの描画
		std::vector<scr_object::ScrObjectPtr> drawable_objects;

		drawable_objects = this->screens.cur()->get_drawable();

		for(int i = 0; i < drawable_objects.size(); i++){
			POINT dPos = {drawable_objects[i]->get_pos_x(), drawable_objects[i]->get_pos_y()};
			POINT dSize = {drawable_objects[i]->get_size_x(), drawable_objects[i]->get_size_y()};

			disp_lib::copyImage(this->get_image(), drawable_objects[i]->get_image(),
				dPos.x, dPos.y, 0, 0, dSize.x, dSize.y, this->screens.cur()->get_direction());
		}

		//描画必要フラグ全クリア
		for(int i = 0; i < need_draw_objects.size(); i++){
			need_draw_objects[i]->clear_need_draw();
		}

		update_tft = 1;
	}else{
		//一部描画の処理

		//各種オブジェクトの描画

		for(int i = 0; i < need_draw_objects.size(); i++){
			need_draw_objects[i]->clear_need_draw();

			POINT dPos = {need_draw_objects[i]->get_pos_x(), need_draw_objects[i]->get_pos_y()};
			POINT dSize = {need_draw_objects[i]->get_size_x(), need_draw_objects[i]->get_size_y()};

			disp_lib::copyImage(this->get_image(), need_draw_objects[i]->get_image(),
				dPos.x, dPos.y, 0, 0, dSize.x, dSize.y, this->screens.cur()->get_direction());
		}

		//再描画アイテムが１つ以上なら
		if(need_draw_objects.size() > 0){
			update_tft = 1;
		}
	}

	//定期呼び出し処理実行
	this->screens.cur()->call_interval_extra();

	if(update_tft){
		//TFTへ画素を転送
		display::onDraw();
	}
}
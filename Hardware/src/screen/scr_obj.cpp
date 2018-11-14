#include "scr_obj.hpp"

#include "disp_lib.hpp"

#include <algorithm>

//scr_objectの実装
scr_object::scr_object(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y)
{
	this->position.x = pos_x;
	this->position.y = pos_y;

	if(size_x == 0) size_x = 1;
	if(size_y == 0) size_y = 1;

	this->size.x = size_x;
	this->size.y = size_y;

	this->need_draw = DRAW_NO;

	this->obj_image = gdImageCreateTrueColor(this->size.x, this->size.y);

	//基本色は白で、タッチされたら灰色
	this->attributes["color"] = gdTrueColor(255, 255, 255);
	this->attributes["push_color"] = gdTrueColor(128, 128, 128); 

	//基本深度は０
	this->attributes["depth"] = 0;

	//オブジェクト画像初期化
	scr_object::onPushUpDraw();

	this->visible = 0;
	this->enable = 0;

	this->object_id = obj_id;
	this->type = "scr_object";

	this->onPushUpExtra = NULL;
	this->onPushDownExtra = NULL;

	this->parent_screen = NULL;
}

scr_object::ScrObjectPtr scr_object::create(std::string obj_id, int pos_x, int pos_y, int size_x, int size_y)
{
	return scr_object::ScrObjectPtr(new scr_object(obj_id, pos_x, pos_y, size_x, size_y));
}

scr_object::~scr_object()
{
	gdImageDestroy(this->obj_image);
}

scr_object* scr_object::set_parent(screen* parent_scr)
{
	this->parent_screen = parent_scr;

	return this;
}

screen* scr_object::get_parent()
{
	return this->parent_screen;
}

std::string scr_object::get_id()
{
	return this->object_id;
}

std::string scr_object::get_type()
{
	return this->type;
}

gdImagePtr scr_object::get_image()
{
	return this->obj_image;
}

int scr_object::get_size_x()
{
	return this->size.x;
}

int scr_object::get_size_y()
{
	return this->size.y;
}

int scr_object::get_pos_x()
{
	return this->position.x;
}

int scr_object::get_pos_y()
{
	return this->position.y;
}

scr_object* scr_object::set_pos(int x, int y)
{
	this->position.x = x;
	this->position.y = y;

	return this;
}

scr_object* scr_object::set_size(int x, int y)
{
	this->size.x = x;
	this->size.y = y;

	//キャンバスを再作成
	gdImageDestroy(this->obj_image);
	//オブジェクト画像を作成
	this->obj_image = gdImageCreateTrueColor(this->size.x, this->size.y);

	//オブジェクト画像初期化
	this->onPushUpDraw();

	return this;
}

scr_object* scr_object::clear_need_draw()
{
	this->need_draw = DRAW_NO;

	return this;
}

scr_object* scr_object::set_need_draw(int value)
{
	this->need_draw = value;

	return this;
}

int scr_object::get_need_draw()
{
	return this->need_draw;
}

int scr_object::get_visible()
{
	return this->visible;
}

int scr_object::get_enable()
{
	return this->enable;
}

scr_object* scr_object::set_visible(int visible)
{
	if(this->object_id != ""){
		this->visible = visible;

		//描画必要フラグセット
		this->need_draw = DRAW_CLEAR;
	}

	return this;
}

scr_object* scr_object::set_enable(int enable)
{
	if(this->object_id != ""){
		this->enable = enable;
	}

	return this;
}

scr_object* scr_object::set_attr(std::string name, boost::any value)
{
	std::map<std::string, boost::any>::iterator it;

	//キー名が既に存在するかチェック
	it = this->attributes.find(name);
	if(it != this->attributes.end()){
		//次は型が一致するかチェック
		if((*it).second.type() == value.type()){
			(*it).second = value;

			//設定変更時のメソッド呼び出し
			this->onChangeSetting();
		}
	}

	return this;
}

boost::any* scr_object::get_attr(std::string name)
{
	std::map<std::string, boost::any>::iterator it;

	it = this->attributes.find(name);

	//名前が一致したら返す
	if(it != this->attributes.end()){
		return &((*it).second);
	}else{
		return NULL;
	}
}

const std::type_info* scr_object::get_attr_type(std::string name)
{
	std::map<std::string, boost::any>::iterator it;

	it = this->attributes.find(name);

	//名前が一致したら型情報を返す
	if(it != this->attributes.end()){
		return &((*it).second.type());
	}else{
		return NULL;
	}
}

void scr_object::call_pushup_extra()
{
	//ダミーなら何もしない
	if(this->object_id == "") return;

	if(this->onPushUpExtra != NULL){
		this->onPushUpExtra(this);
	}
}

void scr_object::call_pushdown_extra()
{
	//ダミーなら何もしない
	if(this->object_id == "") return;

	if(this->onPushDownExtra != NULL){
		this->onPushDownExtra(this);
	}
}

scr_object* scr_object::set_pushup_extra(std::function<void (scr_object*)> func)
{
	this->onPushUpExtra = func;

	return this;
}

scr_object* scr_object::set_pushdown_extra(std::function<void (scr_object*)> func)
{
	this->onPushDownExtra = func;

	return this;
}

void scr_object::onPushUp()
{
	//タッチが離れたら元の状態にする
	this->onPushUpDraw();

	this->call_pushup_extra();
}

void scr_object::onPushDown()
{
	//タッチされたらタッチ色で塗りつぶす
	this->onPushDownDraw();

	this->call_pushdown_extra();
}

//描画情報が変更されたときに呼び出される
void scr_object::onChangeSetting()
{
	this->onPushUpDraw();
}

void scr_object::onPushUpDraw()
{
	int color = boost::any_cast<int>(this->attributes["color"]);

	//オブジェクト非タッチ状態の描画
	gdImageFilledRectangle(this->obj_image, 0, 0,
		this->size.x, this->size.y, color);

	//描画必要フラグセット
	this->need_draw = DRAW_OBJECT;
}

void scr_object::onPushDownDraw()
{
	int push_color = boost::any_cast<int>(this->attributes["push_color"]);

	//オブジェクトタッチ状態の描画
	gdImageFilledRectangle(this->obj_image, 0, 0,
		this->size.x, this->size.y, push_color);

	//描画必要フラグセット
	this->need_draw = DRAW_OBJECT;
}

//screenの実装
screen::screen(std::string screen_id)
{
	this->screen_id = screen_id;
	this->object_pushup = NULL;

	//スクリーンの基本色は白
	this->color = gdTrueColor(255, 255, 255);

	this->direction = DIRECTION_NORMAL;

	this->onLoadExtra = NULL;
	this->onUnloadExtra = NULL;
	this->onIntervalExtra = NULL;

	this->dummy_object = scr_object::create("", 0, 0, 1, 1);
}

screen::ScreenPtr screen::create(std::string screen_id)
{
	return screen::ScreenPtr(new screen(screen_id));
}

screen::~screen()
{

}

std::string screen::get_id()
{
	return this->screen_id;
}

//指定されたIDのオブジェクト本体を返す関数
scr_object::ScrObjectPtr screen::get_object(std::string object_id)
{
	//リストのどこにあるか検索
	std::map<std::string, scr_object::ScrObjectPtr>::iterator it 
		= this->scr_object_list.find(object_id);

	//見つかったらそれを返す
	if(it != this->scr_object_list.end()){
		return (*it).second;
	}else{
		//見つからなかったらダミーを返す
		return this->dummy_object;
	}
}

screen* screen::add(scr_object::ScrObjectPtr object)
{
	//オブジェクトIDが重複していないかチェック
	if(this->get_object(object->get_id())->get_id() != ""){
		throw(object->get_id() + ":" + std::string("オブジェクトIDが重複しています"));
	}

	//objectの親を設定
	object->set_parent(this);
	//連想配列に追加
	this->scr_object_list[object->get_id()] = object;

	//順序リストも追加
	this->obj_order_list.push_back(object);

	//これはオブジェクトのdepthが同値の場合の順序保持用マップ
	this->obj_order_index_list[object->get_id()] = -(this->obj_order_list.size() - 1);

	//追加後順序を並び替え
	std::sort(this->obj_order_list.begin(), this->obj_order_list.end(),
			[this](scr_object::ScrObjectPtr left, scr_object::ScrObjectPtr right){
				if(left->get_attr("depth") && right->get_attr("depth")){
					int left_depth = boost::any_cast<int>(*(left->get_attr("depth")));
					int right_depth = boost::any_cast<int>(*(right->get_attr("depth")));

					if(left_depth != right_depth){
						return (left_depth > right_depth);
					}else{
						//深さ値が同等なら前の順序を維持
						return (this->obj_order_index_list[left->get_id()] > this->obj_order_index_list[right->get_id()]);
					}
				}else{
					return false;
				}
			}
	);


	//全再描画フラグセット
	if(obj_order_list.size() > 0){
		obj_order_list[0]->set_need_draw(DRAW_CLEAR);
	}

	return this;
}

screen* screen::remove(std::string object_id)
{
	//リストのどこにあるか検索
	std::map<std::string, scr_object::ScrObjectPtr>::iterator it 
		= this->scr_object_list.find(object_id);

	//見つかったら削除
	if(it != this->scr_object_list.end()){
		this->scr_object_list.erase(it);
	}

	//オブジェクト順序リストのアイテムも削除
	std::vector<scr_object::ScrObjectPtr>::iterator o_it
		= std::find_if(this->obj_order_list.begin(), this->obj_order_list.end(),
			[&object_id](scr_object::ScrObjectPtr obj) { return (object_id == obj->get_id()); });

	//見つかったら削除
	if(o_it != this->obj_order_list.end()){
		this->obj_order_list.erase(o_it);
	}

	//オブジェクト順序インデックスも削除
	std::map<std::string, int>::iterator oi_it
		= this->obj_order_index_list.find(object_id);

	//見つかったら削除
	if(oi_it != this->obj_order_index_list.end()){
		this->obj_order_index_list.erase(oi_it);
	}


	//全再描画フラグセット
	if(obj_order_list.size() > 0){
		obj_order_list[0]->set_need_draw(DRAW_CLEAR);
	}

	return this;
}

screen* screen::set_color(int color)
{
	this->color = color;

	//全再描画フラグセット
	if(obj_order_list.size() > 0){
		obj_order_list[0]->set_need_draw(DRAW_CLEAR);
	}

	return this;
}

int screen::get_color()
{
	return this->color;
}

screen* screen::set_direction(int direction)
{
	this->direction = direction;

	//全再描画フラグセット
	if(obj_order_list.size() > 0){
		obj_order_list[0]->set_need_draw(DRAW_CLEAR);
	}

	return this;
}

int screen::get_direction()
{
	return this->direction;
}

void screen::call_load_extra()
{
	//ダミーなら何もしない
	if(this->screen_id == "") return;

	if(this->onLoadExtra != NULL){
		this->onLoadExtra(this);
	}
}

void screen::call_unload_extra()
{
	//ダミーなら何もしない
	if(this->screen_id == "") return;

	if(this->onUnloadExtra != NULL){
		this->onUnloadExtra(this);
	}
}

void screen::call_interval_extra()
{
	//ダミーなら何もしない
	if(this->screen_id == "") return;

	if(this->onIntervalExtra != NULL){
		this->onIntervalExtra(this);
	}
}

screen* screen::set_load_extra(std::function<void (screen*)> func)
{
	this->onLoadExtra = func;

	return this;
}

screen* screen::set_unload_extra(std::function<void (screen*)> func)
{
	this->onUnloadExtra = func;

	return this;
}

screen* screen::set_interval_extra(std::function<void (screen*)> func)
{
	this->onIntervalExtra = func;

	return this;
}

//指定されたオブジェクト名の要素を取得
std::vector<scr_object::ScrObjectPtr> screen::get_object_list(std::string obj_type)
{
	std::vector<scr_object::ScrObjectPtr> result;

	std::vector<scr_object::ScrObjectPtr>::iterator f_it, next_it;

	//リストの最後に到達するまで検索し、返却リストに追加する
	next_it = this->obj_order_list.begin();
	do {
		//表示有効なアイテムを取得
		f_it = std::find_if(next_it, this->obj_order_list.end(),
				[&obj_type](scr_object::ScrObjectPtr obj) { return (obj_type == "" || obj->get_type() == obj_type); });

		next_it = f_it;

		if(f_it != this->obj_order_list.end()){
			result.push_back(*f_it);

			++next_it;
		}
	} while(f_it != this->obj_order_list.end());

	return result;
}

std::vector<scr_object::ScrObjectPtr> screen::get_drawable()
{
	std::vector<scr_object::ScrObjectPtr> result;

	std::vector<scr_object::ScrObjectPtr>::iterator f_it, next_it;

	//リストの最後に到達するまで検索し、返却リストに追加する
	next_it = this->obj_order_list.begin();
	do {
		//表示有効なアイテムを取得
		f_it = std::find_if(next_it, this->obj_order_list.end(),
				[](scr_object::ScrObjectPtr obj) { return obj->get_visible() != 0; });

		next_it = f_it;

		if(f_it != this->obj_order_list.end()){
			result.push_back(*f_it);

			++next_it;
		}
	} while(f_it != this->obj_order_list.end());

	return result;
}

std::vector<scr_object::ScrObjectPtr> screen::get_need_draw()
{
	std::vector<scr_object::ScrObjectPtr> result;
	std::vector<scr_object::ScrObjectPtr>::iterator f_it, next_it;

	//まずは、再描画必要オブジェクトに被るオブジェクトにも再描画必要フラグをセット
	//この処理は再描画対象増加が無くなるまで繰り返す
	int update_cnt = 0;

	do {
		update_cnt = 0;

		next_it = this->obj_order_list.begin();
		do {
			//再描画が必要なアイテムを取得
			f_it = std::find_if(next_it, this->obj_order_list.end(),
					[](scr_object::ScrObjectPtr obj) { return obj->get_need_draw() != DRAW_NO; });

			next_it = f_it;

			if(f_it != this->obj_order_list.end()){
				std::vector<scr_object::ScrObjectPtr>::iterator sub_f_it, sub_next_it;

				sub_next_it = this->obj_order_list.begin();

				scr_object::ScrObjectPtr src_obj = *f_it;
				do {
					//再描画が必要なアイテムを取得
					sub_f_it = std::find_if(sub_next_it, this->obj_order_list.end(),
							[this, &src_obj](scr_object::ScrObjectPtr obj) {
								//このオブジェクトが既に描画必要ならfalseを返す
								if(obj->get_need_draw() != DRAW_NO) return false;

								//比較元の範囲を格納
								POINT s_area[2][2] = {
									{
										{src_obj->get_pos_x(), src_obj->get_pos_y()},
										{src_obj->get_pos_x() + src_obj->get_size_x(), src_obj->get_pos_y() + src_obj->get_size_y()}
									}, {
										{obj->get_pos_x(), obj->get_pos_y()},
										{obj->get_pos_x() + obj->get_size_x(), obj->get_pos_y() + obj->get_size_y()}
									}
								};

								//比較する四角の頂点の座標を格納
								//格納は左上,右上,右下,左下
								RECT tx_pos[2] = {
									{
										obj->get_pos_x(), obj->get_pos_x() + obj->get_size_x(),
										obj->get_pos_x() + obj->get_size_x(), obj->get_pos_x()
									}, {
										src_obj->get_pos_x(), src_obj->get_pos_x() + src_obj->get_size_x(),
										src_obj->get_pos_x() + src_obj->get_size_x(), src_obj->get_pos_x()
									}
								};
								RECT ty_pos[2] = {
									{
										obj->get_pos_y(), obj->get_pos_y(),
										obj->get_pos_y() + obj->get_size_y(), obj->get_pos_y() + obj->get_size_y()
									}, {
										src_obj->get_pos_y(), src_obj->get_pos_y(),
										src_obj->get_pos_y() + src_obj->get_size_y(), src_obj->get_pos_y() + src_obj->get_size_y()
									}
								};

								//比較先オブジェクトの頂点が、比較元のエリアに入っているかチェック
								int in_flag = 0;
								for(int t_index = 0; t_index < 2; t_index++){
									POINT* s_ptr[2] = {&s_area[t_index][0], &s_area[t_index][1]};
									RECT* tx_ptr = &tx_pos[t_index];
									RECT* ty_ptr = &ty_pos[t_index];

									//頂点の座標を格納
									POINT tpos_table[4] = {
										{tx_ptr->left, ty_ptr->left},
										{tx_ptr->top, ty_ptr->top},
										{tx_ptr->right, ty_ptr->right},
										{tx_ptr->bottom, ty_ptr->bottom}
									};

									//頂点が範囲に入っているか
									for(int p_index = 0; p_index < sizeof(tpos_table) / sizeof(POINT); p_index++){

										if((s_ptr[0]->x <= tpos_table[p_index].x && s_ptr[0]->y <= tpos_table[p_index].y)
											&& (s_ptr[1]->x > tpos_table[p_index].x && s_ptr[1]->y > tpos_table[p_index].y))
										{
											in_flag = 1;
										}

										if(in_flag) break;
									}

									//次に、辺のみが範囲に入っている場合のパターンをチェック
									if(s_ptr[0]->y > tpos_table[0].y && s_ptr[1]->y < tpos_table[2].y){
										//縦長で辺のみ入っている場合

										//先にy比較したから次はx比較
										if(s_ptr[0]->x <= tpos_table[0].x && s_ptr[1]->x > tpos_table[0].x){
											in_flag = 1;
										}
										if(s_ptr[0]->x <= tpos_table[1].x && s_ptr[1]->x > tpos_table[1].x){
											in_flag = 1;
										}
									}
									if(s_ptr[0]->x > tpos_table[0].x && s_ptr[1]->x < tpos_table[1].x){
										//横長で辺のみ入っている場合

										//先にx比較したから次はy比較
										if(s_ptr[0]->y <= tpos_table[0].y && s_ptr[1]->y > tpos_table[0].y){
											in_flag = 1;
										}
										if(s_ptr[0]->y <= tpos_table[2].y && s_ptr[1]->y > tpos_table[2].y){
											in_flag = 1;
										}
									}

									if(in_flag) break;
								}


								//再描画フラグがclearでないなら順序による再描画抑制を行う
								if(src_obj->get_need_draw() != DRAW_CLEAR){
									std::string src_obj_id = src_obj->get_id();
									std::string tg_obj_id = obj->get_id();

									//比較元の深さを取得
									std::vector<scr_object::ScrObjectPtr>::iterator src_it
										= std::find_if(this->obj_order_list.begin(), this->obj_order_list.end(),
											[&src_obj_id](scr_object::ScrObjectPtr obj) { return (src_obj_id == obj->get_id()); });

									//比較先の深さを取得
									std::vector<scr_object::ScrObjectPtr>::iterator tg_it
										= std::find_if(this->obj_order_list.begin(), this->obj_order_list.end(),
											[&tg_obj_id](scr_object::ScrObjectPtr obj) { return (tg_obj_id == obj->get_id()); });

									if(src_it != this->obj_order_list.end() &&
										tg_it != this->obj_order_list.end())
									{
										//両方の深さを比較し比較元が大きければ再描画しない
										if(src_it > tg_it){
											in_flag = 0;
										}
									}
								}


								if(in_flag){
									return true;
								}else{
									return false;
								}
							});

					sub_next_it = sub_f_it;

					if(sub_f_it != this->obj_order_list.end()){
						++update_cnt;

						(*sub_f_it)->set_need_draw(DRAW_OBJECT);

						++sub_next_it;
					}
				} while(sub_f_it != this->obj_order_list.end());


				++next_it;
			}
		} while(f_it != this->obj_order_list.end());

	} while(update_cnt > 0);


	//次に再描画フラグがセットされているものを取得
	next_it = this->obj_order_list.begin();
	do {
		//再描画が必要なアイテムを取得
		f_it = std::find_if(next_it, this->obj_order_list.end(),
				[](scr_object::ScrObjectPtr obj) { return obj->get_need_draw() != DRAW_NO; });

		next_it = f_it;

		if(f_it != this->obj_order_list.end()){
			//このオブジェクトが表示可能か再描画フラグがDRAW_CLEARなら
			if((*f_it)->get_visible()){
				result.push_back(*f_it);
			}else if((*f_it)->get_need_draw() == DRAW_CLEAR){
				result.push_back(*f_it);
			}else{
				//それ以外なら再描画フラグクリア
				(*f_it)->clear_need_draw();
			}

			++next_it;
		}
	} while(f_it != this->obj_order_list.end());

	return result;
}

void screen::send_pushup()
{
	if(this->object_pushup != NULL){
		this->object_pushup->onPushUp();

		this->object_pushup = NULL;
	}
}

void screen::send_pushdown(int x, int y)
{
	//pushupが設定してあったらそれにpushdown送信
	if(this->object_pushup != NULL){
		this->object_pushup->onPushDown();
		return;
	}

	//座標の一致する最後尾の要素を取得
	std::vector<scr_object::ScrObjectPtr>::reverse_iterator f_it = 
		std::find_if(this->obj_order_list.rbegin(), this->obj_order_list.rend(),
			[&x, &y](scr_object::ScrObjectPtr obj){

				//オブジェクト表示状態かつ座標に入ってるなら
				if(obj->get_visible()){
					if((obj->get_pos_x() <= x && obj->get_pos_y() <= y) &&
						(obj->get_pos_x() + obj->get_size_x() > x && obj->get_pos_y() + obj->get_size_y() > y))
					{
						return true;
					}
				}

				return false;
			}
	);

	if(f_it != this->obj_order_list.rend()){
		//このオブジェクトが有効なら
		if((*f_it)->get_enable()){
			this->object_pushup = *f_it;

			(*f_it)->onPushDown();
		}
	}
}

//screen_containerの実装
screen_container::screen_container()
{
	this->dummy_screen = screen::create("");
	this->current_screen = this->dummy_screen;

	this->onCreateExtra = NULL;
	this->onDestroyExtra = NULL;
	this->onSelectExtra = NULL;
	this->onLoadedExtra = NULL;
}

screen_container::~screen_container()
{
	//クラス消失前に最後のカレントスクリーンのunload関数実行
	this->current_screen->call_unload_extra();

	//クラス消失前にscreen_containerのdestroy呼び出し
	this->call_destroy_extra();
}

void screen_container::call_create_extra()
{
	if(this->onCreateExtra != NULL){
		this->onCreateExtra();
	}
}

void screen_container::call_destroy_extra()
{
	if(this->onDestroyExtra != NULL){
		this->onDestroyExtra();
	}
}
void screen_container::call_select_extra(screen* scr)
{
	if(this->onSelectExtra != NULL){
		this->onSelectExtra(scr);
	}
}
void screen_container::call_loaded_extra(screen* scr)
{
	if(this->onLoadedExtra != NULL){
		this->onLoadedExtra(scr);
	}
}

screen_container* screen_container::set_create_extra(std::function<void ()> func)
{
	this->onCreateExtra = func;

	return this;
}
screen_container* screen_container::set_destroy_extra(std::function<void ()> func)
{
	this->onDestroyExtra = func;

	return this;
}
screen_container* screen_container::set_select_extra(std::function<void (screen*)> func)
{
	this->onSelectExtra = func;

	return this;
}
screen_container* screen_container::set_loaded_extra(std::function<void (screen*)> func)
{
	this->onLoadedExtra = func;

	return this;
}

screen::ScreenPtr screen_container::get_screen(std::string screen_id)
{
	//リストのどこにあるか検索
	std::map<std::string, screen::ScreenPtr>::iterator it =
		this->screen_list.find(screen_id);

	//見つかったらそれを返す
	if(it != this->screen_list.end()){
		return (*it).second;
	}else{
		//見つからなかったらダミーを返す
		return this->dummy_screen;
	}
}

screen_container* screen_container::add(screen::ScreenPtr scr)
{
	//スクリーンIDが重複していないかチェック
	if(this->get_screen(scr->get_id())->get_id() != ""){
		throw(scr->get_id() + ":" + std::string("スクリーンIDが重複しています"));
	}

	this->screen_list[scr->get_id()] = scr;

	return this;
}

screen_container* screen_container::remove(std::string screen_id)
{
	//リストのどこにあるか検索
	std::map<std::string, screen::ScreenPtr>::iterator it =
		this->screen_list.find(screen_id);

	//見つかったら削除
	if(it != this->screen_list.end()){
		this->screen_list.erase(it);
	}

	return this;
}

screen_container* screen_container::select(std::string screen_id)
{
	screen::ScreenPtr scr_ptr = this->get_screen(screen_id);

	if(scr_ptr->get_id() != ""){
		//変更前に前のスクリーンのunload関数呼び出し
		this->current_screen->call_unload_extra();

		//このコンテナ自身のselectイベント呼び出し
		this->call_select_extra(scr_ptr.get());

		//変更後のスクリーンにload関数呼び出し
		scr_ptr->call_load_extra();

		//このコンテナ自身のloadedイベント呼び出し
		this->call_loaded_extra(scr_ptr.get());

		this->current_screen = scr_ptr;
	}

	return this;
}

screen::ScreenPtr screen_container::cur()
{
	return this->current_screen;
}

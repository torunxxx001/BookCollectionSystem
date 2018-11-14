//モールシステム用メインJavaScript

var def_page_jump_mode = 1; // ショップ一覧のページ移動動作モード 0:戻るボタン非考慮 1:戻るボタン考慮
var def_scrollbar_width = 0; // スクロールバーの幅　自動設定されるので設定不要
var def_shopcell_width = 292; // ショップ一覧の１ショップあたりのセル幅
var def_shopcell_height = 200; // ショップ一覧の1ショップあたりのセル高
var def_main_content_width = 0;  // main-contentの幅（０なら自動）
var def_shopcell_rows = 5; // ショップ一覧の一列あたりのショップ数
var def_user_id = null // ログインしていたら自動でユーザＩＤが格納される

var def_bbs_per_page = 100;  //掲示板の１ページに表示される件数

var def_admin_mode = false;  //adminモードフラグ　自動セット
var def_usr_url = null; //ユーザショップへのURL　自動セット

var def_footer_text = '2012.4.10 徐研究室'; //フッターに表示するテキスト

var shop_params = {  // getshopsで使用されるパラメタ
	'cell_cols':0, //店舗一覧の列数
	'q':null, //店舗検索でのキーワード
	'limit':0, //店舗検索での返却上限数
	'offset':0, //店舗検索での返却位置オフセット
	'order':'bow asc', //店舗検索での並び替え
	'mode':'' //現在の店舗一覧がどういうモードなのか(カテゴリなのか・通常検索なのか・マイイイね！リストなのか・・・)
};

//関数定義スタート

// コンテンツの幅を自動調整する関数
// 試行錯誤で確定した内容なので詳細説明不可・・
function chk_cont_width(call_reload){
	//各種幅取得
	var window_width = $(window).width() - def_scrollbar_width;
	var outer_width = $('#outer').outerWidth();
	var menu_width = $('#menu').outerWidth();
	var right_cont_width = $('#right-content').outerWidth();

	var cell_cols = 0;
	if(def_main_content_width == 0){
		//def_main_content未指定の場合

		//外部空白を取得
		var margins = window_width - outer_width;
		//コンテンツの幅を計算
		var cont_width = outer_width;
		cont_width -= (menu_width + right_cont_width);
		cont_width += margins;

		//１セルあたりの幅を格納
		var shop_cell_width_tmp = def_shopcell_width + 20;

		cell_cols = Math.floor(cont_width / shop_cell_width_tmp);
		if(cell_cols < 2) cell_cols = 2;

		//列セル数が変わるかどうか
		if(shop_params['q'] != null && shop_params['cell_cols'] != cell_cols){
				//店舗一覧の上限を 列数 x デフォルト行数
				shop_params['limit'] = cell_cols * def_shopcell_rows;
				shop_params['cell_cols'] = cell_cols;

				//列セル数が変わったらリロード
				if(call_reload) get_shops();
			
		}

		//メインコンテンツの幅を計算
		var main_cont_width = shop_cell_width_tmp * cell_cols;
	}else{
		//def_main_content指定の場合

		var main_cont_width = def_main_content_width;
	}

	//outerのサイズを計算
	outer_width = main_cont_width + (cell_cols * 5) + menu_width + right_cont_width + 8;
	$('#outer').width(outer_width);

	//マージンのサイズ検索
	margins = Math.floor((window_width - outer_width) / 2);
	if(margins < 0) margins = 0;
	$('#outer').css({'margin-left':margins, 'margin-right':margins});

	//メインコンテンツの幅を設定
	$('#main-content').width(main_cont_width);

	//店舗一覧列数2かつランキングが表示か非表示かで検索ボックスのサイズを可変
	if(cell_cols == 2 && $('#right-content').data('ranking-hide-flag')){
		$('#search-text').width('25%');
	}else{
		$('#search-text').width('35%');
	}
}

// ランキング項目を処理する関数
function set_ranking(category, target){
	if(target == ''){
		//target未指定の場合どのランキングかは乱数で決める
		var rand = 0;

		if(rand >= 0 && rand < 1000){
			target = 'price';
		}else if(rand >= 1000 && rand < 2000){
			target = 'number';
		}else if(rand >= 2000 && rand < 5000){
			target = 'purenum';
		}else{
			target = 'nicenum';
		}
	}

	//ランキングタイトル
	var rank_name = '';
	switch(target){
		case 'price': rank_name = '売上金額ランキング'; break;
		case 'number': rank_name = '販売数ランキング'; break;
		case 'purenum': rank_name = '純売上数ランキング'; break;
		case 'nicenum': rank_name = 'イイね!数ランキング'; break;
	}

	if(category == '') rank_name += '(総合)';

	var params = {'target':target, 'flag':'1'};

	if(category != '') params['cat'] = category;

	//ランキング読み込み
	$.get('bin/sys/mall_ranking.php', params, function(data){
		var r_con = $('#right-content');

		$('.ranking-title, .ranking-item', r_con).remove();
		r_con.append($('<div>', {'class':'ranking-title'}).text(rank_name));

		//店舗情報の取り出し
		$('data > ranking > shop', data).each(function(){
			var shop_info = {
				'rank': $('rank', this).text(),
				'shop_id': $('shop_id', this).text(),
				'shop_name': $('shop_name', this).text(),
				'user_id': $('user_id', this).text(),
				'prog_name': $('prog_name', this).text(),
				'pic_url': $('pic_url', this).text(),
				'nice': $('nice', this).text(),
				'nice_num': $('nice_num', this).text()
			};

			var rnk_item = $('<div>');

			var anker_1 = $('<a>', {'href':def_usr_url+shop_info['prog_name'], 'target':'_blank'});
			var anker_2 = $('<span>').append(anker_1.clone(true));

			var img_obj = $('<img>', {'class':'shop-image', 'src':shop_info['pic_url'], 'alt':'shop-image'});

			img_obj.error(function(){
				$(this).attr('src', 'img/noimg.gif');
			});
			anker_1.append(img_obj);
			rnk_item.append(anker_1);

			var shop_name = shop_info['shop_name'];

			//店舗名が長い場合は途中で切る
			if(getStrWidth(shop_name) > 220) shop_name = substr_pixel(shop_name, 220) + '...';

			$('a', anker_2).append(shop_name);
			rnk_item.append($('<div>').html(shop_info['rank']+'位'+'<br/>'+anker_2.html()+'<br/>'+shop_info['user_id']+'<span class="rank-nice-area"></span>'));

			$('.rank-nice-area', rnk_item).append('<img class="rank-nice-button">');
			$('.rank-nice-area', rnk_item).append('<img src="img/nice/balloon.png" class="rank-nice-balloon">');
			$('.rank-nice-area', rnk_item).append('<span class="rank-nice-balloon-text"></span>');

			$('.rank-nice-balloon, .rank-nice-balloon-text', rnk_item).hide();

			var nice_num = shop_info['nice_num'];
			var set_nice_num = nice_num;
			if(set_nice_num >= 1000){
				set_nice_num = 999;
			}

			$('.rank-nice-balloon-text', rnk_item).text(set_nice_num);

			var nice_img_url = ['img/nice/nice.png', 'img/nice/del_nice.png'];
			if(shop_info['nice'] == 'nice'){
				$('.rank-nice-button', rnk_item).attr('src', nice_img_url[1]);
			}else{
				$('.rank-nice-button', rnk_item).attr('src', nice_img_url[0]);
			}

			$('.rank-nice-button', rnk_item).button();
			if(shop_info['user_id'] == def_user_id) {
				$('.rank-nice-button', rnk_item).css('opacity', '0.5');
			}

			//イイね!ボタン動作
			$('.rank-nice-button', rnk_item).click(function(){
				if(shop_info['user_id'] == def_user_id) return;

				var img_url = $(this).attr('src');

				var nice_flag = 0;
				if(img_url == nice_img_url[0]){
					nice_flag = 1;
				}

				//イイね!を設定
				$(this).prop('disabled', true);
				var target_img = $(this);
				$.post('bin/sys/mall_nice_act.php', {'mode':'set', 'shop_id':shop_info['shop_id'], 'nice':nice_flag}, function(data){
					var status = $('data > status', data).text();

					target_img.prop('disabled', false);
					if(status == 'ok'){
						target_img.attr('src', nice_img_url[nice_flag]);

						if(nice_flag){
							nice_num++;
						}else{
							nice_num--;
						}
						var set_nice_num = nice_num;
						if(set_nice_num >= 1000){
							set_nice_num = 999;
						}else if(set_nice_num < 0){
							set_nice_num = 0;
						}
						$('.rank-nice-balloon-text', target_img.parent()).text(set_nice_num);
					}
				});
			});

			$('.rank-nice-button', rnk_item).hover(function(){
				$('.rank-nice-balloon', $(this).parent()).show();
				$('.rank-nice-balloon-text', $(this).parent()).show();
			}, function(){
				$('.rank-nice-balloon', $(this).parent()).hide();
				$('.rank-nice-balloon-text', $(this).parent()).hide();
			});

			r_con.append($('<div>', {'class':'ranking-item'}).append(rnk_item));
		});

		//ランキングを非表示にするトグル動作
		var hide_act = function(){
			$('#right-content').data('ranking-hide-flag', 1);

			$('#right-content > .ranking-title, #right-content > .ranking-item').hide();

			//表示動作領域を作成
			$('#right-content')
				.prepend(
					$('<div>', {'id':'rank-show-button'}).html('&lt;&lt;')
					.hover(function(e){
						$(this).css('background-color','orange');
					},function(e){
						$(this).css('background-color','white');
					})
				);

			//ランキング表示動作
			$('#rank-show-button').click(function(e){
				$('#right-content').data('ranking-hide-flag', 0);

				$('#rank-show-button').remove();
				$('#right-content > .ranking-title, #right-content > .ranking-item').show();
				$('#right-content').animate({'width':170}, 'slow', null, function(){
					chk_cont_width(true);
				});
			});

			//店舗一覧の列数調整
			chk_cont_width(true);
		};

		//ランキングタイトルについての動作
		$('#right-content > .ranking-title')
			.hover(function(){
				$(this).css({'background-color':'orange', 'cursor':'pointer'});

				$(this).append($('<span>', {'id': 'rank-hide-button'}).html('&gt;&gt;'));

				$('#rank-hide-button', this).hover(function(){
					$(this).css({'background-color':'brown', 'cursor':'pointer'});

				}, function(){
					$(this).css({'background-color':'white', 'cursor':'pointer'});
				})
				.click(function(e){
					e.stopPropagation();
					$('#right-content:not(:animated)').animate({'width':20}, 'slow', null, hide_act);
				});

			}, function(){
				$(this).css({'background-color':'white', 'cursor':'pointer'});

				$('#rank-hide-button', this).remove();
			})
			.click(function(e){
				//ランキングタイトルがクリックされたらランキング切り替え
				var next_list = {
					'nicenum': 'purenum',
					'purenum': 'price',
					'price': 'number',
					'number': 'nicenum'
				};
				set_ranking(category, next_list[target]);
		});

		//ランキング非表示フラグの動作
		if($('#right-content').data('ranking-hide-flag')){
			if($('#rank-show-button').length <= 0){
				$('#right-content:not(:animated)').width(20);

				hide_act();
			}

			$('#right-content > .ranking-title, #right-content > .ranking-item').hide();
		}else{
			$('#right-content > .ranking-title, #right-content > .ranking-item').show();
		}
	});
}


// ログインボタンを押したときのアクション
function login_button_act(){
	if($('#login-user-id, #login-password').prop('disabled') == true) return;

	$('#login-user-id, #login-password').prop('disabled', true);

	var params = {};
	if($("#login-button").text() == 'ログアウト'){
		var mode = 'logout';
	}else{
		var mode = 'login';

		if($('#login-user-id').val().length <= 0){
			alert('学籍番号を入力してください');
			$('#login-user-id, #login-password').prop('disabled', false);
			return;
		}

		//同期通信で公開鍵取得後、暗号化して変数に代入
		$.ajax({
			async:false,
			type:'get',
			url:'bin/sys/book_auth.php',
			data:{'mode':'get_public_key'},
			success:function(data){
				var pinfo = getPubkeyInfo($('data > public_key', data).text());

				if(pinfo["status"] == "ok"){
					var rsa = new RSAKey();
					rsa.setPublic(pinfo["modulus"], pinfo["exponent"]);

					params['user_id'] = rsa.encrypt($('#login-user-id').val());
					params['password'] = rsa.encrypt($('#login-password').val());
				}else{
					alert(pinfo['status']);
				}
			}
		});

		if(params['user_id'] == null || params['password'] == null) return;
	}

	params['mode'] = mode;

	//ログイン・ログアウト動作を実行
	$.post('bin/sys/book_auth.php', 
		params, 
		function(data){
			if($('data > status', data).text() == 'ok'){
				window.location.reload();
			}else{
				alert($('data > status', data).text());
				$('#login-user-id, #login-password').prop('disabled', false);
			}
		}
	);
}

// ログインフォームの表示を切り替える関数
function login_form_set(user_id){
	if(user_id == ''){
		$('#login-forms').html('学籍番号<input type="text" id="login-user-id"/><br/>' +
					'パスワード<input type="password" id="login-password"/>');

		set_enter_action($('#login-user-id, #login-password'), login_button_act);

		$('#login-button').text('ログイン');
		$('#login-forms').css('vertical-align', 'bottom');
	}else{
		if(user_id.length > 10){
			user_id = user_id.substring(0, 10) + '...';
		}
		var hour = (new Date()).getHours();
		if(hour >= 18 || hour < 6){
			var h_text = 'こんばんは';
		}else{
			var h_text = 'こんにちは';
		}

		$('#login-forms').html('<p>'+h_text+'、'+user_id+'&nbsp;さん</p>');

		$('#login-button').text('ログアウト');
		$('#login-forms').css('vertical-align', 'middle');
	}
}

// ブラウザの戻るボタン考慮時に、URLパラメタに各種情報を乗せる関数
function jump_search_next(){
	if(shop_params['q'] != null){
		var hide_rank_flag = 0;
		if($('#right-content').data('ranking-hide-flag')){
			hide_rank_flag = 1;
		}

		var url_params = '?q=' + escape(shop_params['q']) +
							'&mode=' + shop_params['mode'] +
							'&offset=' + shop_params['offset'] +
							'&rows=' + def_shopcell_rows +
							'&order=' + shop_params['order'] +
							'&hide_rank=' + hide_rank_flag;

		window.location.href = url_params;
	}
}

// ショップ一覧に、１ショップセルの情報を追加する関数
function add_shop_cell(dest_selector, shop_info){
	var shop_cell = $('<div>', {'class':'shop-cell'}).append($('<div>'));

	var img_obj = $('<img>', {'class':'shop-image', 'src':shop_info['pic_url'], 'alt':'shop-image'});
	img_obj.error(function(){
		$(this).attr('src', 'img/noimg.gif');
	});

	var shop_name = shop_info['title'];
	//店舗名文字列幅が420を超えた場合は切り取り
	if(getStrWidth(shop_name) > 420) shop_name = substr_pixel(shop_name, 420) + '...';

	//店舗情報のtimeから日時を取り出し
	var date_str = shop_info['date'].match(/(\d{4})-(\d{2})-(\d{2}).*/);
	date_str = date_str[1]+'/'+date_str[2]+'/'+date_str[3]+' 発売';

	//店舗オーナーを取得
	var owner = shop_info['author'];
	//文字列幅が150を超えた場合は切り取り
	if(getStrWidth(owner) > 300) owner = substr_pixel(owner, 300) + '...';
	

	var description = shop_info['description'];

	//店舗説明文の文字列幅取得
	var desc_resize = 0;
	var str_width = getStrWidth(description);

	if(str_width > 1200) {
		//文字列幅が1200を超えた場合はフォントサイズ変更
		desc_resize = 1;
	}
	if(str_width > 2600){
		//文字列幅が1800を超えた場合は切り取り
		description = substr_pixel(description, 2600) + '...';
	}

	$('div', shop_cell)
		.append(img_obj)
		.append($('<div>', {'class':'shop-name'}).append(shop_name))
		.append($('<div>', {'class':'shop-author'}).append(owner))
		.append($('<div>', {'class':'shop-date'}).append(date_str))
		.append($('<div>', {'class':'shop-description'}))
		.append($('<div>', {'class':'shop-status'}));

	//図書説明文追加
	$('.shop-description', shop_cell)
		.append($('<span>').append(description));

	//図書状態追加
	//ステータス設定
	if(shop_info['status'] == 'exist'){
		$('.shop-status', shop_cell)
			.css('background-color', 'lightblue')
			.append('図書状態：　あり');
	}else{
		$('.shop-status', shop_cell)
			.css('background-color', 'orange')
			.append('図書状態：　貸出中');
	}


	if(desc_resize) $('.shop-description', shop_cell).css('font-size', '8pt');

	$(dest_selector).append(shop_cell);
	$(dest_selector).append($('<div>', {'class':'shopcell-margin'}));

}

//ページャを作る関数
function create_pager(target_selector, cur_page, max_page, cnt_per_page){
	var pager = $('<div>', {'class':'pager'});

	//現在のページ番号位置を基準とした左側右側のページ番号数
	var left_num = 3;
	var right_num = 3;

	//店舗セル数が２超えならばページャのページ数を増やす
	if(shop_params['cell_cols'] > 2){
		left_num += (shop_params['cell_cols'] - 2) * 2;
		right_num += (shop_params['cell_cols'] - 2) * 2;
	}

	//左右ページャ数補正
	if(cur_page <= left_num + 1){
		right_num += (left_num + 1) - cur_page + 1;

		//前のXX件が表示されない分を上限に加算
		if(cur_page == 1) right_num += 3;
	}
	//左右ページャ数補正
	if(cur_page + right_num >= max_page){
		left_num += (right_num + 1) - (max_page-cur_page)

		//次のXX件が表示されない分を上限に加算
		if(cur_page == max_page) left_num += 3;
	}

	//前のXX件出力
	if(cur_page > 1){
		pager.append($('<div>', {'class':'prev-next-page'}).append('前の'+cnt_per_page+'件'));
	}

	//一番左に１ページ目へのリンクを張る
	if(cur_page - left_num > 1){
		pager.append($('<div>').append('1'));
	}

	//現在のページ数より左側のページ番号を出力
	for(var index = cur_page - left_num; index < cur_page; index++){
		if(index > 0) pager.append($('<div>').append(index));
	}
	//現在のページ数を含めた右側のページ番号を出力
	for(var index = cur_page; index <= max_page && index <= cur_page + right_num; index++){
		if(index == cur_page){
			pager.append($('<div>', {'class':'current-page'}).append(index));
		}else{
			pager.append($('<div>').append(index));
		}
	}

	//一番右に最終ページへのリンクを張る
	if(cur_page + right_num < max_page){
		pager.append($('<div>').append(max_page));
	}

	//次のXX件出力
	if(cur_page < max_page){
		pager.append($('<div>', {'class':'prev-next-page'}).append('次の'+cnt_per_page+'件'));
	}

	$(target_selector).append(pager);

	var pager_width = 5;
	$('> .pager > div', $(target_selector)).each(function(index){
		pager_width += $(this).outerWidth() + 5;
	});

	$('> .pager', $(target_selector))
		.height($('> .pager > div:first', $(target_selector)).height() + 15)
		.width(pager_width);

}

// ショップ情報の一覧を、DBから取得する関数
function get_shops(){
	$.get('bin/sys/book_search.php', {'q': shop_params['q'], 'limit':shop_params['limit'], 'offset':shop_params['offset'],'order':shop_params['order']}, function(data){

		$('#main-content').empty();

		//現在の検索ワードでのリミット・オフセット・総店舗数を取得
		var limit = parseInt($('data > info > limit', data).text());
		var offset = parseInt($('data > info > current_offset', data).text());
		var book_count = parseInt($('data > info > book_count', data).text());

		//カテゴリ一覧表示ならば
		if(shop_params['mode'] == 'genre'){
			var match_arr = shop_params['q'].match(/genre:(.*)/);

			if(match_arr){
				var title_str = match_arr[1] + ' (' + book_count + ')';

				var html_str = '<div class="search-result-title">' +
								'ジャンル &gt; '+title_str + 
								'</div>';
			}
		}else{
			//それ以外は通常検索

			search_text = shop_params['q'];
			if(search_text == ''){
				search_text = '全表示';
			}else{
				search_text = '&quot;' + search_text + '&quot;';
			}

			search_text += ' ('+book_count+')';
			var html_str = '<div class="search-result-title">' +
							'検索 &gt; '+search_text + 
							'</div>';
		}

		$('#main-content').html(html_str);


		//最終ページ番号・現在のページ番号・現在のページでの店舗一覧数取得
		var cur_cells = book_count - offset;
		var max_page = Math.ceil(book_count / limit);
		var cur_page = max_page - Math.ceil(cur_cells / limit) + 1;

		//上部ページャを追加
		$('#main-content').append($('<div id="pager-area"></div>'));
		create_pager('#pager-area', cur_page, max_page, shop_params['limit']);

		//ページャの幅を自動調整
		if(cur_cells >= shop_params['cell_cols']){
			$('#pager-area').width(shop_params['cell_cols'] * def_shopcell_width);
		}else{
			$('#pager-area').width(cur_cells * def_shopcell_width);
		}

		if(max_page > 0){
			//並び替えプルダウン追加
			$('#pager-area').append('<div id="pager-select">' +
				'<select id="search-order">' +
				'<option value="bow">貸出状態</option>' +
				'<option value="isbn">ISBN</option>' +
				'<option value="title">タイトル</option>' +
				'<option value="auth">著者</option>' +
				'<option value="pub">出版社</option>' +
				'<option value="desc">解説文</option>' +
				'<option value="genre">ジャンル</option>' +
				'<option value="date">発売日</option>' +
				'<option value="srank">Amazonランク</option>' +
				'<option value="price">価格</option>' +
				'<option value="pages">ページ数</option>' +
				'</select><select id="search-order-opt">' +
				'<option value="asc">昇順</option>' +
				'<option value="desc">降順</option>' +
				'</select></div>');
		}

		//orderパラメタから、現在の並び替え設定をプルダウンメニューに設定
		var order = shop_params['order'].split(' ');

		$('#search-order').val(order[0]);
		$('#search-order-opt').val(order[1]);

		$('#search-order, #search-order-opt').change(function(){
			var order = $('#search-order').val() + ' ' + $('#search-order-opt').val();

			shop_params['order'] = order;
			if(def_page_jump_mode == 0){
				$('html').scrollTop(0);
				get_shops();
			}else{
				jump_search_next();
			}
		});

		$('#main-content').append($('<p>', {'class':'clear-both'}));

		//店舗一覧に店舗セルを追加していく
		$('data > results > shop', data).each(function(index) {
			var book_info = {
				'isbn':$('isbn', this).text(),
				'pic_url':'bin/sys/book_get_cover_img.php?isbn='+$('isbn', this).text(),
				'title':$('Title', this).text(),
				'author':$('Author', this).text(),
				'publisher':$('Publisher', this).text(),
				'description':$('Description', this).text(),
				'genre':$('Genre', this).text(),
				'date':$('Date', this).text(),
				'status':$('book_status', this).text()
			};
			add_shop_cell('#main-content', book_info);
		});

		$('#main-content').append($('<p>', {'class':'clear-both'}));

		//下部ページャを追加
		create_pager('#main-content', cur_page, max_page, shop_params['limit']);

		//ページャがクリックされたときの動作を設定
		$('.pager > div:not(.current-page)').click(function(e){
			if($(this).text().match(/次の.*/)){
				shop_params['offset'] += shop_params['limit'];
			}else if($(this).text().match(/前の.*/)){
				shop_params['offset'] -= shop_params['limit'];
			}else{
				var click_page = parseInt($(this).text()) - 1;
				shop_params['offset'] = shop_params['limit'] * click_page;
			}

			//ページ遷移を伴わせるかどうか
			if(def_page_jump_mode == 0){
				$('html').scrollTop(0);
				get_shops();
			}else{
				jump_search_next();
			}
		});

		//ページャの色設定
		$('.pager > div:not(.current-page)').hover(function(e){
			$(this).css('background-color', 'red');
		}, function(e){
			$(this).css('background-color', 'yellow');
		});
	});
}

//item_textで指定したメニューのクリックイベントを発動させる
function on_menu_click(item_text){
	var menu_events = $('#menu').data('menu_click_events');

	for(var index=0; index < menu_events.length; index++){
		if(menu_events[index]['name'] == item_text){
			menu_events[index]['obj'].trigger('click');
			break;
		}
	}
}

//メニューのクリックイベントを無効にする
function disable_menu(enable_flag){
	if(disable_menu){
		$('#menu').data('disable_flag', 1);
	}else{
		$('#menu').data('disable_flag', 0);
	}
}

//メニューを読み込みDOMに追加した後にクリックイベントを設定
//group_ids:メニューグループの番号
//click_action:メニューがクリックされた場合の動作
// ex.  function(menu_class_name, item_text){

//load_action:メニューが読み込まれた後の動作
// ex.  function(){
function load_menu(group_ids, click_action, load_action){
	//同期で読み込む
	$.ajax({
		async:false,
		type:'get',
		url:'bin/sys/book_get_menu.php',
		success:function(data){
			//読み込みが完了したら・・・

			//再帰で深部メニューを表示
			var show_deep = function(parent, parent_num, depth){
				var subcat = parent.children('submenu');

				if(subcat.length > 0){
					var class_text = 'menu-subitems-' + parent_num + '-' + depth;
					var retn = $('<ul>', {'class':'menu-subitems'}).addClass(class_text);

					subcat.each(function(index){
						var list_item = $('<li>').append($('<span>').append($(this).children('name').text()));

						var child = show_deep($(this), parent_num, depth + 1);
						if(child){
							list_item.append(child);
						}

						retn.append(list_item);
					});
				}else{
					var retn = null;
				}

				return retn;
			};

			//受信したＸＭＬからメニューを作成する
			var max_menu_pixel = 0;
			var cat_items = $('<ul>', {'class':'menu-items'});
			$('data > menu_items > menu', data).each(function(index){
					var cat_item = $('<li>');
					var cat_text = $(this).children("name").text();

					cat_item.append($('<span>').append(cat_text));

					var tmp_pixel = getStrWidth(cat_text);
					if(max_menu_pixel < tmp_pixel){
						max_menu_pixel = tmp_pixel;
					}

					var sub_cats = $('submenu', this);
					if(sub_cats.length > 0){
						cat_item.append(show_deep($(this), index, 0));
					}

					cat_items.append(cat_item);
			});
			$('#menu').append(cat_items);

			//メニューの幅を考慮に入れて、各種アイテムの幅を再計算
			max_menu_pixel = parseInt(max_menu_pixel + 5);
			$('#menu').width(max_menu_pixel);
			$('#contents').css({'margin-left':max_menu_pixel});
			$('#mall-navi').css('min-width', max_menu_pixel);
			$('#mall-navi > #navi-left').width(max_menu_pixel - 2);
			$('#mall-navi > #navi-middle').css('margin-left', max_menu_pixel - 2);


			//サブメニューに対して、メニュー幅調整とボーダー付けを実行
			$('ul.menu-items > li').each(function(index){
				var left_tmp = max_menu_pixel - 2;

				for(var depth = 0; depth < 10; depth++){
					var class_text = '.menu-subitems-'+index+'-'+depth;

					var sub_item = $(class_text+' > li');
					if(sub_item.length <= 0) break;

					var max_smenu_pixel = 0;
					sub_item.each(function(){
						var text_tmp = $(this).children('span').text();

						var tmp_pixel = getStrWidth(text_tmp);
						if(max_smenu_pixel < tmp_pixel){
							max_smenu_pixel = tmp_pixel;
						}

					});

					var pixel_tmp = parseInt(max_smenu_pixel + 5);

					$(class_text).width(pixel_tmp);
					$(class_text).css('left', left_tmp);

					left_tmp = pixel_tmp - 2;
				}
			});

			//メニューのホバーイベント登録
			$('ul.menu-items li').hover(function(){
					$('>ul:not(:animated)', this).slideDown(0);
					$(this).css('background-color', 'white');
			}, function(){
					$('>ul', this).slideUp(0);
					$(this).css('background-color', 'wheat');
			});

			var click_events = [];
			//メニューのクリックイベントを登録
			$('ul.menu-items li').each(function(index){
				var ele_cname = $(this).parent().attr('class');
				var item_text = $(this).children('span').html();
				var parent_text = $('> span', $(this).parents('li:first')).html();
				
				if(ele_cname.match(/menu-subitems/)){
					var sp_item_text = parent_text + ',' + item_text;
				}else{
					var sp_item_text = item_text;
				}

				//メニューがクリックされた時に呼び出される関数
				var click_act = function(e){
					if(ele_cname.match(/menu-items/)){
						//メインメニュークリック時の動作

						$('>ul', this).slideUp(0);
					}else if(ele_cname.match(/menu-subitems/)){
						//サブメニュークリック時の動作

						$(this).children('ul').slideUp(0);
						$(this).parent().slideUp(0);
						$(this).parent().trigger('mouseout');
					}

					if(!$('#menu').data('disable_flag')){
						click_action(ele_cname, sp_item_text);
					}

					e.stopPropagation();
				};


				//クリックイベントリストに追加
				click_events.push({'name':sp_item_text, 'obj':$(this)});

				//メニューがクリックされた場合の動作を設定
				$(this).click(click_act);
			});

			$('#menu').data('menu_click_events', click_events);

			//ロード動作呼び出し
			load_action();
		}
	});
}

// 対象セレクタにタブを追加する関数
// contentsは配列で記述は
// contents[0] = array('title','contents')の形式

function add_tabs(dest_selector, base_id, contents, tab_options){
	var tab_cont = $('<div>', {'id':base_id, 'class':'tab-style-content'});
	var ul_o = $('<ul>', {'class':'tab-style-title'});
	for(var index = 0; index < contents.length; index++){
		var id_num = index + 1;
		ul_o.append(
			$('<li>').append(
				$('<a>', {'href':'#'+base_id+'-'+id_num}).append(contents[index][0])
			)
		);

		tab_cont.append(
			$('<div>', {'class':'add-tabs-tab-content', 'id':base_id+'-'+id_num}).append(contents[index][1])
		);
	}

	tab_cont.prepend(ul_o);

	if(tab_options){
		var options = tab_options;
	}else{
		var options = null;
	}

	$(dest_selector).append(tab_cont);
	$('#'+base_id).tabs(options);
}

//指定したセレクタに、店舗選択を追加
function add_shop_select(target_sel, search_params, click_act){
	//追加引数を参照
	if(arguments.length == 3){
		//追加引数が１個の場合はクリック動作
		var click_act = arguments[2];
	}else if(arguments.length == 4){
		//追加引数が２個の場合は読み込み完了動作＆クリック動作
		var done_act = arguments[2];
		var click_act = arguments[3];
	}

	$(target_sel).append('読み込み中...');

	//店舗一覧を読み込む
	$.get('bin/sys/mall_search.php', search_params, function(data){
		$(target_sel).empty();

		var count = parseInt($('data > info > result_count', data).text());

		if(count > 0){
			var sel_cols = Math.floor($(target_sel).width() / def_shopcell_width);
			$(target_sel).height(Math.ceil(count / sel_cols) * def_shopcell_height);

			var tmp_click_act = function(){
				click_act(this, $(this).data('shop_info'));
			};

			//店舗セルを追加
			$('data > results > shop', data).each(function(index){
				var shop_info = {
					'shop_id':$('shop_id', this).text(),
					'anker':$('<a>', {'href':'javascript:void(0);'}).click(tmp_click_act).data('shop_info', $(this)),
					'pic_url':$('pic_url', this).text(),
					'shop_name':$('shop_name', this).text(),
					'time':$('time', this).text(),
					'user_id':$('user_id', this).text(),
					'description':$('description', this).text(),
					'flag':$('flag', this).text(),
					'nice':$('nice', this).text(),
					'nice_num':$('nice_num', this).text()
				};
				add_shop_cell(target_sel, shop_info);
			});
		}else{
			$(target_sel).append('<strong>店舗なし</strong>');
		}

		if(done_act) done_act(data);
	});
}

//関数定義終了

//メイン処理スタート

//ドキュメントの読み込みが完了したら・・・
$(function(){
	//HTTP通信でキャッシュを無効に
	$.ajaxSetup({'cache': false});

	// 左上のロゴにイベントを登録
	$('#mall-logo-image').hover(function(e){
		$(this).css('cursor', 'pointer');
	}).click(function(e){
		window.location.href = '.';
	});

	// 右上のログインボタンを追加
	create_button('#login-button', 'img/login_btn.gif', login_button_act);


	// ログインステータスを取得してフォームを設定
	$.ajax({ 
		async:false,
		type:'get',
		url:'bin/sys/book_auth.php',
		data:{'mode':'check'},
		success:function(data, dataType){
			if($('data > login_status', data).text() == '0'){
				login_form_set('');
			}else{
				login_form_set($('data > user_id', data).text());

				def_user_id = $('data > user_id', data).text();
				if($('data > admin', data).text() == 'TRUE'){
					def_admin_mode = true;
				}
			}
		}
	});

	// スクロールバーの幅を計算
	def_scrollbar_width = $('html').width() - $('html').prop('clientWidth');
	if(!def_scrollbar_width) def_scrollbar_width = 17;

	var url_params = getURLparams();


	load_menu('0', function(class_name, sp_item_text){
		//カテゴリ名がクリックされた動作が記述

		shop_params['offset'] = 0;
		shop_params['mode'] = 'genre';
		shop_params['q'] = 'genre:'+sp_item_text;

		if(def_page_jump_mode == 0){
			chk_cont_width(false);
			get_shops();

			set_ranking(sp_item_text, '');

			$('#search-text').val('');
		}else{
			jump_search_next();
		}
	}, function(){

		// ウィンドウのリサイズ時の処理（誤動作防止処理を追加済み）
		var lastWidth = 0;
		$(window).resize(function(){
			if(lastWidth != $(window).width()){
				chk_cont_width(true);
				lastWidth = $(window).width();
			}
		});

		// 中段のナビに内容を追加
		$('#mall-navi > #navi-left').html('ジャンル一覧');
		$('#mall-navi > #navi-middle').html(
						'<input type="text" id="search-text"/>' +
						'<input type="button" id="search-btn" value="検索"/>' +
						'<a href="javascript:void(0);" id="search-dialog-open">詳細検索</a>'
		);
		// 検索ボタンをスタイリッシュに
		$('#search-btn').button();

		if(def_user_id != null) {
			//簡単ログイン用バーコード生成
			$('#mall-navi > #navi-right').html('<input type="button" id="gen-login-bk-btn" value="ログインバーコード作成"/>');
			$('#gen-login-bk-btn')
				.button()
				.click(function(){



					window.open('bin/sys/gen_login_barcode.php', 'login_barcode', 'width=400, height=300, menubar=yes, toolbar=no, scrollbars=no, resizable=yes');
				});
		}

		// 検索時の動作
		var search_act = function(){
			if(def_user_id == null) return;

			$('#search-text').blur();
			var search_text = $('#search-text').val().toLowerCase();

			shop_params['offset'] = 0;
			shop_params['mode'] = 'search';
			shop_params['q'] = search_text;

			if($('#search-text').data('dialog-open-flag')){
				chk_cont_width(false);
				get_shops();
			}else{
				if(def_page_jump_mode == 0){
					chk_cont_width(false);
					get_shops();
					set_ranking('', '');
				}else{
					jump_search_next();
				}
			}
		};

		// 検索ボタンのクリックや検索欄のエンター時の動作を設定
		$('#search-btn').click(search_act);

		set_enter_action($('#search-text'), search_act);

		// 詳細検索のクエリ追加ボタンクリック時の動作
		var add_search_query = function(){
			if(!$('#sd-search-target').data('set-hint') && $('#sd-search-text').val().length > 0){

				var search_val = $('#search-text').val();
				if($('#search-text').val().length > 0){
					search_val += ' ' + $('#sd-search-andor').val() + ' ';
				}
				var search_target = $('#sd-search-target').val();
				var search_opt = '';
				if(search_target != 'date'){
					search_opt = $('#sd-search-opt').val();
				}
				if(search_target.length > 0) {
					search_target = search_target + ':';
				}

				search_val += search_target + search_opt + $('#sd-search-text').val();

				$('#search-text').val(search_val);
				$('#sd-search-text').val('');

				$('#search-btn').trigger('click');
			}
		};

		// ダイアログの内容をボディに追加する
		$('body').append(
			$('<div>', {'id':'search-dialog'})
				.append(
					$('<div>')
						.append($(
							'<span>対象：</span><select id="sd-search-target">' +
							'<option value="">すべて</option>' +
							'<option value="isbn">ISBN</option>' +
							'<option value="genre">ジャンル</option>' +
							'<option value="title">タイトル</option>' +
							'<option value="auth">著者</option>' +
							'<option value="pub">出版社</option>' +
							'<option value="desc">解説文</option>' +
							'<option value="date">発行日</option>' +
							'</select><span>から</span>' +
							'<input type="text" id="sd-search-text"/>' +
							'<span>が</span><select id="sd-search-opt">' +
							'<option value="">含まれる</option>' +
							'<option value="^">前方一致する</option>' +
							'<option value="$">後方一致する</option>' +
							'<option value="!">完全一致する</option>' +
							'</select>' +
							'<input type="button" id="sd-clear-text" value="検索欄クリア">' +
							'<select id="sd-search-andor">' +
							'<option value="">AND</option>' +
							'<option value="or">OR</option>' +
							'</select><span>の</span>' +
							'<input type="button" id="sd-add-text-btn" value="クエリを追加"/>'
						))
				)
		);

		// 検索欄クリアボタンの動作を設定
		$('#sd-clear-text').click(function(e){
			$('#search-text').val('');
		});
		// 詳細検索の検索欄でエンターを押したときの動作を設定
		set_enter_action($('#sd-search-text'), function(){
			$('#sd-search-target').data('set-hint', 0);
			add_search_query();
		});

		//	クエリを追加ボタンを押したときの動作を設定
		$('#sd-add-text-btn').click(function(e){
			add_search_query();
		});

		// 詳細検査の検索欄のヒント関連の動作
		$('#sd-search-text')
			.focus(function(){
				$(this).css('color','black');
				if($('#sd-search-target').data('set-hint')){
					$(this).val('');
				}
			})
			.blur(function(){
				if($(this).val().length == 0){
					$(this).css('color','gray');
					$(this).val($('#sd-search-target').data('hint'));
					$('#sd-search-target').data('set-hint', 1);
				}else{
					$('#sd-search-target').data('set-hint', 0);
				}
			});

		// 検索対象のプルダウンメニューを変更したときの動作
		$('#sd-search-target').change(function(){
			if($(this).val() == 'date'){
				$(this).data('hint', 'YYYY/MM-YYYY/MM');
			}else{
				$(this).data('hint', 'キーワードを入力');
			}

			if($(this).data('set-hint')){
				$('#sd-search-text').val('');
				$('#sd-search-text').trigger('blur');
			}
		});

		// 詳細検索の検索対象を、初めはすべてにしておく
		// その後ヒントを設定
		$('#sd-search-target').data('set-hint', 1);
		$('#sd-search-target').trigger('change');

		// 詳細検索ダイアログのオブジェクトを非表示で作成
		$('#search-dialog').dialog({'modal':true,'width':440,'height':130,'title':'詳細検索','autoOpen':false, 'resizable':false});
		
		// 詳細検索ダイアログクローズ時の動作を設定
		$('#search-dialog').dialog({'close':function(){
			$('#search-text').data('dialog-open-flag', 0);

			if($('#search-text').val().length > 0 && def_page_jump_mode == 1) search_act();
		}});

		// 詳細検索ダイアログを、詳細検索文字列クリック時に開くよう設定する
		$('#search-dialog-open').click(function(e){
			if(def_user_id == null) return;

			$('#search-text').data('dialog-open-flag', 1);
			$('#search-dialog').dialog('open');
			set_special_dialog_close($('#search-dialog'));
			return false;
		});

		// URLのパラメタを取得して、そのパラメタの内容の通りに表示する
		var url_params = getURLparams();
		//qが入っていたら店舗一覧表示
		if(def_user_id != null && url_params['q'] != null){
			var query = unescape(url_params['q']);

			shop_params['q'] = query;
			if(query.indexOf('genre:') >= 0 && url_params['mode'] == 'genre'){
				shop_params['mode'] = 'genre';

				var str_tmp = query.slice(query.indexOf('genre:') + 4);

				set_ranking(str_tmp, '');
			}else if(url_params['mode'] == 'nice_list'){
				shop_params['mode'] = 'nice_list';

				$('#search-text').val(query);
				set_ranking('', '');
			}else{
				shop_params['mode'] = 'search';

				$('#search-text').val(query);
				set_ranking('', '');
			}

			if(url_params['offset'] && parseInt(url_params['offset']) > 0){
				shop_params['offset'] = parseInt(url_params['offset']);
			}
			if(url_params['rows'] && parseInt(url_params['rows']) > 0){
				def_shopcell_rows = parseInt(url_params['rows']);
			}
			if(url_params['order']){
				shop_params['order'] = url_params['order'];
			}
			if(parseInt(url_params['hide_rank'])){
				$('#right-content').data('ranking-hide-flag', 1);
			}

			chk_cont_width(false);
			//店舗一覧表示
			get_shops();
		}else{
			chk_cont_width(false);
			//ランキングをランダムで表示(総合、ランダム)
			set_ranking('', '');

			if(def_user_id != null){
				var explain = 'ここでは、このシステム内に登録された図書を検索することが出来ます。<br/>'+
					'また、同時にその図書の貸し出し情報を見ることが出来ます。<br/><br/>' +
					'<strong>図書を検索するには・・・</strong><br>' +
					'左側のカテゴリをクリックするか、上の検索ボックスで検索ワードを入力してください。<br>';
			}else{
				var explain = '<div style="height:200px;"><strong>まずは、右上のログインフォームからログインしてください。</strong></div>';
			}

			//スタートページを設定
			var tab_cont = [];

			tab_cont.push(['スタートページ', 
				'<div class="tab-content">' +
				'ようこそ蔵書管理システムへ<br><br>' +
				explain +
				'</div>'
			]);

			add_tabs('#main-content', 'main-cont', tab_cont);
		}

	}); //メニューロード後のイベント終了

$('#right-content').width(0).hide();
	//未ログインならばすべての機能を無効化
	if(def_user_id == null){
		disable_menu(1);
		$('#right-content').width(0).hide();
		chk_cont_width(false);
		$('#go-manager').hide();
		$('#go-mall').hide();
	}
});
//メイン処理終了
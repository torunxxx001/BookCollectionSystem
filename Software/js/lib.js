// JavaScript用汎用ライブラリ

//BODYの幅を取得
function getBodyWidth(){
	return window.innerWidth || (document.documentElement || document.body).clientWidth;
}
//BODYの高さを取得
function getBodyHeight(){
	return window.innerHeight || (document.documentElement || document.body).clientHeight;
}

//文字列を指定したピクセルの幅に収める関数
function substr_pixel(text, pixel){
	var eID = 'chk-slen-' + (new Date()).getTime();
	var eObj = $('<div>', {'id': eID});

	eObj.css('visibility', 'hidden');
	eObj.width(100000);

	eObj.append($('<span>'));
	$('body').append(eObj);

	var span_sel = '#'+eID + '> span';
	$(span_sel).text(text);
	var swidth = $(span_sel).width();

	var tmp_text = text;
	if(swidth > pixel){
		var old_text = '';

		endf = 0;
		while(1){
			old_text = tmp_text;
			tmp_text = tmp_text.substring(0, tmp_text.length / 2);
			if(tmp_text.length <= 0) break;

			$(span_sel).text(tmp_text);
			var tmp_width = $(span_sel).width();

			if(tmp_width <= pixel) break;
		}

		if(tmp_text.length > 0){
			tmp_text = old_text;

			while(1){
				tmp_text = tmp_text.substring(0, tmp_text.length - 1);
				if(tmp_text.length <= 0) break;

				$(span_sel).text(tmp_text);
				var tmp_width = $(span_sel).width();

				if(tmp_width <= pixel) break;
			}
		}
	}

	$('#'+eID).remove();

	return tmp_text;
}

//文字列の幅をピクセルサイズで取得する関数
function getStrWidth(text){
	var eID = 'chk-slen-' + (new Date()).getTime();
	var eObj = $('<div>', {'id': eID});

	eObj.css('visibility', 'hidden');
	eObj.width(100000);

	eObj.append($('<span>'));
	$('body').append(eObj);

	var span_sel = '#'+eID+' > span';
	$(span_sel).text(text);
	var swidth = $(span_sel).width();

	$('#'+eID).remove();

	return swidth;
}

// テキストボックス上でエンターを押したときに
// 処理が実行されるように、テキストボックスを設定する関数
function set_enter_action(text_obj, action){
	text_obj.keydown(function(e) {
		if(e.keyCode == 13){
			action(this);
		}
	});
}

// jQuery-ui-dialog用の閉じる動作拡張
// modal:true時しか動作しない
// 動作としては、ダイアログ外の黒くなった部分をクリックしたらダイアログ
// が閉じるというもの
function set_special_dialog_close(dialog_obj){
	$('.ui-widget-overlay').click(function(e){
		var offset = dialog_obj.offset();
		var width = dialog_obj.outerWidth();
		var height = dialog_obj.outerHeight();
		var titlebar_height = $('.ui-dialog-titlebar').outerHeight();

		dialog_obj.dialog('close');
		$('.ui-widget-overlay').unbind('click');
	});
}

// URLにパラメタが指定されていたら、そのパラメタを抜き出す関数
function getURLparams(){
	var hash_params = new Array();
	var url = window.location.href;

	if(url.indexOf('?') >= 0){
		var tmp_text = unescape(url.slice(url.indexOf('?') + 1));
		var params = tmp_text.split('&');

		for(var index in params){
			var param_data = params[index].split('=');

			if(param_data.length < 2) param_data[1] = '';

			hash_params[index] = param_data[1];
			hash_params[param_data[0]] = param_data[1];
		}
	}

	return hash_params
}

// あらかじめ用意されたボタン画像から、ボタンを作成する
function create_button(button_id, img_url, event){
	$(button_id).css({'cursor':'pointer', 'background-image':'url("'+img_url+'")'});
	$(button_id).mousedown(function(e){
		$(this).css('background-position', '0 -50px');
	}).mouseup(function(e){
		$(this).css('background-position', '0 -25px');
		event();
	});

	$(button_id).hover(function(e){
		$(this).css('background-position', '0 -25px');
	}, function(e){
		$(this).css('background-position', '0 0');
	});
}

// 後ろのpxという文字が入っているピクセルサイズを数値に変換して返す
function removePx(string){
	var ret = 0;
	if(string){
		var px_index = string.lastIndexOf('px');

		if(px_index >= 0){
			ret = parseInt(string.substring(0, px_index));
		}else{
			ret = parseInt(string);
		}

	}
	return ret;
}

// 文字長をバイト単位で計算する関数
function str_bytelength(string){
	var esc_str = escape(string);

	var length = 0;
	for(var index = 0; index < esc_str.length; index++, length++){
		if(esc_str.charAt(index) == '%'){
			if(esc_str.charAt(index + 1) == 'u'){
				index += 3;
				length++;
			}

			index += 2;
		}
	}

	return length;
}

//バイト文字単位で文字列を切り取る関数
function substring_byte(string, str_start, str_length){
	var esc_str = escape(string);
	var esc_tmp = '';

	var length = 0;
	for(var index = 0; index < esc_str.length; index++, length++){
		if(esc_str.charAt(index) == '%'){
			var len_tmp = 2;
			if(esc_str.charAt(index + 1) == 'u'){
				len_tmp += 3;
				length++;
			}

		}else{
			var len_tmp = 0;
		}

		if(length + 1 > str_length) break;
		esc_tmp += esc_str.substring(index, index + len_tmp + 1);

		index += len_tmp;
	}

	return unescape(esc_tmp);
}

//内容をリスト表示して確認を求める関数
function list_confirm(list_array, message, callback){
	var callback_called = false;

	$('<div id="confirm-dialog">' +
		'<strong>'+message+'</strong><hr>' +
		'<div id="item-list"></div>' +
		'<hr><br><input type="button" id="confirm-ok" value="OK">&nbsp;&nbsp;' +
		'<input type="button" id="confirm-cancel" value="キャンセル">' +
		'</div>'
	).dialog({
		'modal':true,
		'width': 400,
		'height': 350,
		'close':function(){
			$(this).dialog('destroy');
			$(this).remove();
			
			if(!callback_called) callback(false);
		}
	});

	$(list_array).each(function(index){
		$('#item-list').append(this + '').append($('<br>'));
	});
	
	$("#confirm-ok, #confirm-cancel").click(function(e){
		if($(this).val() == 'OK'){
			callback(true);
		}else{
			callback(false);
		}
		callback_called = true;
		
		$('#confirm-dialog').dialog('close');
		
	});
}
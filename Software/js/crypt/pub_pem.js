//オリジナル公開鍵用ライブラリ

//PEM形式の公開鍵から、モジュラスとエクスポーネントを抽出する関数
function getPubkeyInfo(pubKey){

	var prefix = '-----BEGIN PUBLIC KEY-----';
	var suffix = '-----END PUBLIC KEY-----';

	var keyPref = pubKey.indexOf(prefix);
	var keySuff = pubKey.lastIndexOf(suffix);
	if(keyPref < 0 || keySuff < 0) return {'status':'公開鍵が正しくありません'};

	var b64_key = pubKey.substring(keyPref+prefix.length, keySuff);

	var bin_key = b64toBA(b64_key);

	var sign = '';
	var modulus = '';
	var exponent = '';
	//ASN.1解析関数
	var tag_search = function(data, idx){
		var index = idx;

		for(; index < data.length;){
			//タグ,構造化フラグ,クラス取得
			var tag = data[index] & 0x1F;
			var struct_flag = (data[index] & 0x20) >> 5;
			var class_flag = (data[index] & 0xC0) >> 6;
			index++;

			//値サイズが128オクテット以上か
			var size_num = 1;
			if(data[index] > 0x80){
				size_num = data[index] - 0x80;
				index++;
			}
			var size=0; //値サイズ変数
			//値サイズ加算
			for(var scnt = 0; scnt < size_num; scnt++){
				size += data[index++];

				if(scnt+1 < size_num) size <<= 8;
			}

			var sub_index = index;

			//タグ別処理
			switch(tag){
				case 0x03:
				//BIT STRING

					//未使用ビット数
					var unused_bit = data[sub_index++];

					//BIT STRINGを配列に格納
					var sub_data = [];
					for(; sub_index < index + size; sub_index++){
						sub_data.push(data[sub_index]);
					}

					//未使用ビットを削る
					for(; unused_bit > 7; unused_bit -= 8){
						sub_data.pop();
					}
					sub_data[sub_data.length - 1] >>= unused_bit;

					//BIT STRINGを再帰
					tag_search(sub_data, 0);
					break;
				case 0x02:
				//INTEGER

					//数値を取得
					var int_str = '';
					for(; sub_index < index + size; sub_index++){
						var num = data[sub_index].toString(16);
						if(num.length < 2) num = '0'+num;
						int_str += num;
					}

					if(int_str.length > 6){
						//数値の長さが６超えならばモジュラス

						modulus = int_str;
					}else{
						//それ以外ならエクスポーネント

						exponent = int_str;
					}
					break;
				case 0x06:
				//OBJECT IDENTIFIER

					//OBJECT IDENTIFIERを変換
					var id = [];
					var tmp_num = 0;
					for(; sub_index < index + size; sub_index++){
						var num = data[sub_index];

						//最初のバイトは別処理
						if((sub_index - index) == 0){
							id.push(parseInt(num / 40));
							id.push(num % 40);
						}else{
							if(num & 0x80){
								tmp_num += num - 0x80;
								tmp_num <<= 7;
							}else{
								tmp_num += num;
								id.push(tmp_num);
								tmp_num = 0;
							}
						}
					}

					//IDENTIFIERを格納
					sign = id.join('.');
					break;
			}

			//構造化フラグが１なら再帰
			if(struct_flag){
				index = tag_search(data, index);
			}

			//indexにサイズを加算
			index += size;
		}

		return index;
	}

	tag_search(bin_key, 0);

	if(sign.indexOf('1.2.840.113549.1.1.1') < 0){
		return {'status':'公開鍵の識別IDが正しくありません'};
	}

	if(modulus == ''){
		return {'status':'モジュラスの取得に失敗しました'};
	}
	if(exponent == ''){
		return {'status':'エクスポーネントの取得に失敗しました'};
	}

	return {'status':'ok', 'modulus':modulus, 'exponent':exponent};
}
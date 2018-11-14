<?php
//暗号化を提供するライブラリ

// MEMO 使用できるモードは、CBCのみ
//		useIV == TRUEの場合はIVを使用して暗号化し、IVを暗号化して返す　リターンはarray(暗号文、暗号化されたIV)
//		useIV == FALSEの場合はIVを使用せずに暗号化する　リターンは　暗号文

//暗号・復号初期化関数
function crypt_init($key, $genIV, $defIV)
{
	//AES-192ビット暗号化CBCモードを利用
	$td = mcrypt_module_open("rijndael-192", "", "cbc", "");
	if(!$td) throw new Exception("module open error");

	//キーをSHA1文字列にし、バイナリ変換
	$keyHash = pack("H*", sha1($key));

	//IVサイズを取得
	$ivSize = mcrypt_enc_get_iv_size($td);

	if($genIV){
		//IVを生成する場合
		$iv = mcrypt_create_iv($ivSize, MCRYPT_RAND);
	}else if($defIV !== NULL){
		//IVを設定する場合
		$iv = $defIV;
	}else{
		//IVを利用しない場合
		$iv = str_repeat("0", $ivSize);  //IV ダミー
	}

	//MCRYPTを初期化しハンドルを取得
	$ret = mcrypt_generic_init($td, $keyHash, $iv);

	if($ret < 0 || $ret === FALSE) throw new Exception("mcrypt init error");

	return array("td" => $td, "iv" => $iv);
}

//暗号・復号開放関数
function crypt_deinit($arr_td)
{
	mcrypt_generic_deinit($arr_td["td"]);
	mcrypt_module_close($arr_td["td"]);
}

//暗号化する関数
function encrypt($data, $password, $useIV = FALSE)
{
	try{
		//初期化
		$arr_td = crypt_init($password, $useIV, NULL);

		//入力値を暗号化し１６進数文字列で返す
		$crypted = mcrypt_generic($arr_td["td"], $data);
		$enc_data = bin2hex($crypted);

		if($useIV){
			//IVを利用する場合は利用したIVを暗号化する
			$iv_td = crypt_init($password, FALSE, NULL);

			$iv_crypted = mcrypt_generic($iv_td["td"], $arr_td["iv"]);
			$iv_enc_data = bin2hex($iv_crypted);

			crypt_deinit($iv_td);
		}

		crypt_deinit($arr_td);
	}catch(Exception $e){
		return NULL;
	}

	if($useIV){
		//IV利用の場合は配列で情報を返す
		return array("data" => $enc_data, "iv" => $iv_enc_data);
	}else{
		//IV未使用の場合は暗号化データのみ返す
		return $enc_data;
	}
}

//復号化する関数
function decrypt($enc_data, $password, $crypted_iv = NULL)
{
	try{
		$dec_iv = NULL;
		if($crypted_iv !== NULL){
			//暗号化したIVが入力された場合は復号化して利用する
			$iv_td = crypt_init($password, FALSE, NULL);
			$bin_data = pack("H*", $crypted_iv);

			$dec_iv = mdecrypt_generic($iv_td["td"], $bin_data);

			crypt_deinit($iv_td);
		}

		//復号化されたIVを利用して情報を復号化
		$arr_td = crypt_init($password, FALSE, $dec_iv);
		$bin_data = pack("H*", $enc_data);

		$dec_data = mdecrypt_generic($arr_td["td"], $bin_data);

		crypt_deinit($arr_td);
	}catch(Exception $e){
		return NULL;
	}

	return $dec_data;
}

?>
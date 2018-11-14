<?php

require_once("../../conf/conf.php");
require_once("../../lib/dblib.php");
require_once("../../lib/syslib.php");

session_start();

$xml_data = <<<XML_END
<?xml version="1.0" encoding="UTF-8"?>
<data>
XML_END;

$status = "ok";

try{
	$mode = $_GET["mode"];
	if(!isset($_GET["mode"])){
		$mode = $_POST["mode"];
	}

	if($mode !== "login" && $mode !== "logout"
		&& $mode !== "check" && $mode !== "get_public_key"
	){
		throw new Exception("モード指定が正しくありません");
	}

	$dbcon = connect_db($CONNECT_DB_PARAM, "book_sys");
	if(!$dbcon) throw new Exception("DBの接続失敗");

	if($mode === "logout"){
		if($_SESSION["book_login_status"] === "logged in"){
			//ログアウト動作
			$_SESSION = array();
		    setcookie(session_name(), "", time() - 42000, "/");
			session_destroy();
		}else{
			throw new Exception("ログインされていません");
		}
	}else if($mode === "login"){
		if(!isset($_POST["user_id"]) || !isset($_POST["password"])){
			throw new Exception("user_idとpasswordの両方を指定してください");
		}else if(strlen($_POST["user_id"]) <= 0){
			throw new Exception("user_idが入力されていません");
		}

		if($_SESSION["book_login_status"] === "logged in"){
			if($_SESSION["user_id"] === $_POST["user_id"]){
				throw new Exception("すでにログインしています");
			}
		}

		//ユーザIDとパスワードの１６進数テキストをバイナリに変換
		$enc_user_id = pack("H*", $_POST["user_id"]);
		$enc_password = pack("H*", $_POST["password"]);

		//サーバに保存された秘密鍵をデコード
		$pkey_res = openssl_pkey_get_private($_SESSION["enc_private_key"], $PRIVATE_KEY_PASSWORD);
		$_SESSION["enc_private_key"] = NULL;

		//暗号化されたユーザIDとパスワードを復号
		$result = openssl_private_decrypt($enc_user_id, $user_id, $pkey_res);
		if(!$result) throw new Exception("ユーザIDの復号化に失敗しました");
		$result = openssl_private_decrypt($enc_password, $password, $pkey_res);
		if(!$result) throw new Exception("パスワードの復号化に失敗しました");

		$user_id = sql_escape_string($dbcon, $user_id);
		$pass_sha1 = sha1($password);

		//ユーザIDから認証タイプを取得
		$sql_str = "SELECT password FROM user_list WHERE user_id='{$user_id}';";
		$sql_result = sql_query($dbcon, $sql_str);
		if(!$sql_result) throw new Exception("認証タイプ問い合わせ失敗");
		$result_arr = sql_fetch_assoc($sql_result);
		sql_free_result($sql_result);

		if(!$result_arr) throw new Exception("ユーザが存在しません");

		if(strtolower($result_arr["password"]) !== strtolower($pass_sha1)) throw new Exception("認証に失敗しました");

		$_SESSION["user_id"] = $user_id;
		$_SESSION["book_login_status"] = "logged in";
	}else if($mode === "check"){
		if($_SESSION["book_login_status"] === "logged in"){
			$xml_data .= "<login_status>1</login_status>";
			$xml_data .= "<user_id>{$_SESSION['user_id']}</user_id>";

			if(check_admin($dbcon, $_SESSION["user_id"])){
				$xml_data .= "<admin>TRUE</admin>";
			}else{
				$xml_data .= "<admin>FALSE</admin>";
			}
		}else{
			$xml_data .= "<login_status>0</login_status>";
		}
	}else if($mode === "get_public_key"){
		if($_SESSION["book_login_status"] === "logged in") throw new Exception("すでにログインされています");

		//秘密鍵を作成
		$pkey = openssl_pkey_new(array('private_key_bits' => 1024));
		//秘密鍵を暗号化して出力
		openssl_pkey_export($pkey, $pkey_string, $PRIVATE_KEY_PASSWORD);

		//秘密鍵から公開鍵を取得
		$pubKey = openssl_pkey_get_details($pkey)["key"];

		//秘密鍵をサーバに保存
		$_SESSION["enc_private_key"] = $pkey_string;
		//公開鍵を出力
		$xml_data .= "<public_key>{$pubKey}</public_key>";
	}

	close_db($dbcon);
}catch(Exception $e){
	if($dbcon) close_db($dbcon);

	$status = $e->getMessage();
}

$xml_data .= <<<XML_END
<status>{$status}</status>
</data>
XML_END;

header("Content-Type: application/xml");
print($xml_data);
?>
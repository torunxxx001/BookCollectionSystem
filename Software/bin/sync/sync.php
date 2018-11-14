<?php

require_once("../../conf/conf.php");
require_once("../../lib/lib.php");
require_once("../../lib/dblib.php");
require_once("../../lib/crypt.php");

session_start();

$xml_data = <<<XML_END
<?xml version="1.0" encoding="UTF-8"?>
<data>
XML_END;

$status = "ok";

try{
	$dbcon = connect_db($CONNECT_DB_PARAM, "book_sys");
	if(!$dbcon) throw new Exception("DBの接続失敗");

	//sql_listを受信して復号化
	$sql_list = $_POST["sql_list"];

	$sql_list = decrypt($sql_list, $SYNC_PASSWORD);

	$sql_list = explode(",", $sql_list);

	//最初の文字がSYNC_SQL_LISTかチェック
	if($sql_list[0] !== "SYNC_SQL_LIST"){
		throw new Exception("同期用SQLのデータが不正です");
	}

	unset($sql_list[0]);

	foreach($sql_list as $sql_str){
		//base64デコード&トリム
		$sql_str = base64_decode($sql_str);
		$sql_str = trim($sql_str);

		if(strlen($sql_str) > 0){
			//SQLの実行
			sql_query($dbcon, $sql_str);
		}
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
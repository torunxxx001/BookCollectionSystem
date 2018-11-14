<?php
require_once("../../conf/conf.php");
require_once("../../lib/dblib.php");


header("Content-Type: application/xml");
print('<?xml version="1.0" encoding="UTF-8"?>');
print("<data>");

$status = "ok";

try{
	$dbcon = connect_db($CONNECT_DB_PARAM, "book_sys");
	if(!$dbcon) throw new Exception("DBの接続失敗");

	$sql_res = sql_query($dbcon, "SELECT Genre FROM book_list GROUP BY Genre;");
	if(!$sql_res) throw new Exception("ジャンル一覧取得失敗");

	print("<menu_items>");
	if(sql_num_rows($sql_res) == 0){
		print("<menu><name>図書情報未登録</name></menu>");
	}else{
		while($row = sql_fetch_assoc($sql_res)){
			print("<menu><name>{$row['Genre']}</name></menu>");
		}
	}
	sql_free_result($sql_res);
	print("</menu_items>");

	close_db($dbcon);
}catch(Exception $e){
	if($dbcon) close_db($dbcon);

	$status = $e->getMessage();
}

print("<status>{$status}</status>");
print("</data>");
?>
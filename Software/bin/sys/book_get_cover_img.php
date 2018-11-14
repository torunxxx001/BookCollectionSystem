<?php
require_once("../../conf/conf.php");
require_once("../../lib/dblib.php");

session_start();

$dbcon = connect_db($CONNECT_DB_PARAM, "book_sys");
if(!$dbcon) die("DBの接続失敗");


if($_SESSION["book_login_status"] !== "logged in") die("ログインしてください");

$isbn = $_GET["isbn"];
if(!isset($isbn) || strlen($isbn) == 0) die("ISBNを指定してください");
if( !is_numeric($isbn)) die("ISBNは数値で入力してください");

$sql_str = "SELECT data FROM image_list WHERE isbn = {$isbn}";

$sql_res = sql_query($dbcon, $sql_str);
if(!$sql_res){
	close_db($dbcon);
	die("SQL問い合わせ失敗");
}


$cover_img_data = "";
//カバー画像取得
if(sql_num_rows($sql_res) > 0){
	while ($row = sql_fetch_assoc($sql_res)){
		$cover_img_data = $row["data"];
	}
}else{
	$cover_img_data = file_get_contents("../../img/noimg.gif");
}
sql_free_result($sql_res);

close_db($dbcon);


//画像出力
switch(exif_imagetype($cover_img_data)){
	case IMAGETYPE_GIF: header("Content-Type: image/gif"); break;
	case IMAGETYPE_JPEG: header("Content-Type: image/jpeg"); break;
	case IMAGETYPE_BMP: header("Content-Type: image/bmp"); break;
	case IMAGETYPE_PNG: header("Content-Type: image/png"); break;
}
print $cover_img_data;
?>
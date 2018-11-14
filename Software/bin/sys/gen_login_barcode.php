<?php
session_start();
require_once("../../conf/conf.php");
require_once("../../lib/lib.php");
require_once("../../lib/dblib.php");

require_once("Image/Barcode2.php");

try{
	if($_SESSION["book_login_status"] !== "logged in") throw new Exception("ログインしてください");
	$user_id = $_SESSION["user_id"];

	$dbcon = connect_db($CONNECT_DB_PARAM, "book_sys");
	if(!$dbcon) throw new Exception("DBの接続失敗");


	$esc_user_id = sql_escape_string($dbcon, $user_id);
	$sql_str = "SELECT MD5(SHA1(CONCAT(user_id, password))) AS estr FROM user_list WHERE user_id = '{$esc_user_id}';";
	$sql_res = sql_query($dbcon, $sql_str);
	if( !$sql_res) throw new Exception("ユーザの問い合わせに失敗しました");
	$res_data = sql_fetch_assoc($sql_res);
	sql_free_result($sql_res);

	$estr = $res_data["estr"];
	$pp = popen("../../lib/bin/gen_code128 {$estr}", "r");
	$estr = fread($pp, 1024);
	pclose($pp);

	$barcode_len = strlen($estr);

	$barcode1 = "-".substr($estr, 0, $barcode_len / 2);
	$barcode2 = "*".substr($estr, $barcode_len / 2);

	//バーコード出力
	header("Content-Type: image/png");
	$ib2 = new Image_Barcode2();
	$bar_img1 = $ib2->draw($barcode1, "code128", "png", false);
	$bar_img2 = $ib2->draw($barcode2, "code128", "png", false);

	$left_img1 = 0;
	$left_img2 = 0;
	if(ImageSX($bar_img1) > ImageSX($bar_img2)){
		$img_width = ImageSX($bar_img1);

		$left_img2 = (ImageSX($bar_img1) - ImageSX($bar_img2)) / 2;
	}else{
		$img_width = ImageSX($bar_img2);

		$left_img1 = (ImageSX($bar_img2) - ImageSX($bar_img1)) / 2;
	}
	$img_height = ImageSY($bar_img1) + ImageSY($bar_img2);

	$image = ImageCreateTrueColor($img_width, $img_height);
	ImageFilledRectangle($image, 0, 0, $img_width, $img_height, 0xFFFFFF);

	ImageCopy($image, $bar_img1, $left_img1, 0, 0, 0, ImageSX($bar_img1), ImageSY($bar_img1));
	ImageCopy($image, $bar_img2, $left_img2, ImageSY($bar_img1), 0, 0, ImageSX($bar_img2), ImageSY($bar_img2));

	ImagePng($image);

	close_db($dbcon);
}catch(Exception $e){
	if($dbcon) close_db($dbcon);

	header("Content-Type: text/plain");
	print("error:".$e->getMessage());
}


?>
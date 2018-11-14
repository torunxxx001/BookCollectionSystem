#!/usr/bin/php
<?php
require_once(dirname(__FILE__) . "/conf.php");
require_once(dirname(__FILE__) . "/crypt.php");
require_once(dirname(__FILE__) . "/http_lib.php");

require_once(dirname(__FILE__) . "/host_list.php");

//標準入力からリストを取得
$sql_list = file_get_contents("php://stdin");

//先頭に認識用文字列挿入
$sql_list = "SYNC_SQL_LIST," . $sql_list;

//暗号化
$enc_sql_list = encrypt($sql_list, $SYNC_PASSWORD);

//データを送信
$http = new HttpClass;

$send_params = array(
	"post_data" => "sql_list={$enc_sql_list}"
);

$headers = array(
			"Content-Type" => "application/x-www-form-urlencoded",
			"Content-Length" => strlen($send_params["post_data"])
);


$http->SetHeaders($headers);
//ホストの分だけ実行
foreach($SYNC_SQL_HOST_LIST as $host_url){
	$http->HttpAction($host_url, "POST", $send_params);
}

?>
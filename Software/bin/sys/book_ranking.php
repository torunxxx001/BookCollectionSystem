<?php
//ランキングを集計するプログラム
/*
	利用テーブル:
		shop
		category
		sales_history
		nice_list

	パラメタ
	param1: GET:span - 集計する期間(未指定の場合全期間)
	param2: GET:cat - 集計するカテゴリ(未指定の場合全店舗)
	param3: GET:shop_id - 指定するとその店舗の商品売り上げランキングになる
	param4: GET:flag - このフラグに一致する店舗が集計される(未指定だと全店舗、1だと表示店舗対象、0だと非表示店舗対象)
	param5: GET:target - 集計対象
		list1: price		-売り上げ金額で集計
		list2: number		-売り上げ数で集計
		list3: purenum		-純売り上げ数で集計
		list4: nicenum		-イイね！数で集計(param3: shop_idを指定した場合は無効)
	param6: GET:order - 順位オーダー
		list1: DESC		- 降順（デフォルト）
		list2: ASC		- 昇順
	param7: GET:limit - 順位の上限数
*/

session_start();

require_once("../../conf/mall_config_system.php");
require_once("../../lib/lib.php");
require_once("../../lib/stm_db_lib.php");

header("Content-Type: application/xml");
print('<?xml version="1.0" encoding="UTF-8"?>');
print("<data>");
$status = "ok";


try{
	if($_SESSION["mall_login_status"] !== "logged in"){
		throw new Exception("ログインしてください");
	}
	$user_id = $_SESSION["user_id"];

	$dbcon = connect_db($SYS_CONNECT_DB_PARAM, "mall_sys");
	if(!$dbcon) throw new Exception("DBの接続失敗");

	$esc_user_id = sql_escape_string($dbcon, $user_id);

	//SPANの期間(2012/12/01-2013/12/01)のようなフォーマットをMySQL用へ変換
	$span = $_GET["span"];
	if(!isset($span)) $span = "";
	if($span !== ""){
		$orig_word = $span;
		if(!preg_match("/(.*)-(.*)/u", $orig_word, $tmp_match)){
			$o_dates = array($orig_word);
		}else{
			$o_dates = array($tmp_match[1], $tmp_match[2]);
		}

		$split_dates = array();
		$dates_count = array_count($o_dates);
		for($index = 0; $index < $dates_count; $index++){
			if(preg_match("/^(\d{4})\\/([0-9]+)\\/([0-9]+)$/u", $o_dates[$index], $tmp_match)
				|| preg_match("/^(\d{4})$/u", $o_dates[$index], $tmp_match)
				|| preg_match("/^(\d{4})\\/([0-9]+)$/u", $o_dates[$index], $tmp_match)
				|| preg_match("/^([0-9]+)\\/([0-9]+)$/u", $o_dates[$index], $tmp_match)
			){
				$arr_cnt = array_count($tmp_match);
				if($arr_cnt > 3){
					$d_str = "{$tmp_match[1]}/{$tmp_match[2]}/{$tmp_match[3]}";
					$intv_str = "+ INTERVAL 1 DAY";
				}else if($arr_cnt > 2){
					if((int)$tmp_match[1] > 100){
						$d_str = "{$tmp_match[1]}/{$tmp_match[2]}/01";
						$intv_str = "+ INTERVAL 1 MONTH";
					}else{
						$d_str = gmdate('Y')."/{$tmp_match[1]}/{$tmp_match[2]}";
						$intv_str = "+ INTERVAL 1 DAY";
					}
				}else{
					$d_str = "{$tmp_match[1]}/01/01";
					$intv_str = "+ INTERVAL 1 YEAR";
				}

				$split_dates[] = sql_escape_string($dbcon, $d_str);
			}else{
				throw new Exception("日付フォーマットが正しくありません");
			}
		}

		$spd_index = 1;
		if($dates_count == 1) $spd_index = 0;

		$time_col_name = "sales_history.time";
		$span = "({$time_col_name} >= DATE('{$split_dates[0]}') AND {$time_col_name} < DATE('{$split_dates[$spd_index]}') {$intv_str})";
	}

	//カテゴリ指定
	$category = $_GET["cat"];
	if(!isset($category)){
		$category = "";
	}else{
		$category = sql_escape_string($dbcon, $category);
		$category = "category.cat_name LIKE '{$category}'";
	}

	//店舗ID指定
	$shop_id = $_GET["shop_id"];
	if(!isset($shop_id) || !is_numeric($shop_id)){
		$shop_id = "";
	}else{
		//店舗IDが自分の店かどうかチェック
		$sql_str = "SELECT shop_id FROM shop WHERE shop_id = {$shop_id} AND user_id = '{$esc_user_id}';";
		$sql_res = sql_query($dbcon, $sql_str);
		if(!$sql_res) throw new Exception("店舗の問い合わせに失敗しました");
		$res_num = sql_num_rows($sql_res);
		sql_free_result($sql_res);
		if($res_num <= 0){
			throw new Exception("対象の店舗はあなたの所有ではありません");
		}

		$shop_id = "shop.shop_id={$shop_id}";
	}

	$sql_sel = "sales_history.shop_id, shop.user_id, shop.pic_url, shop.prog_name, category.cat_name, shop.shop_name, shop.description, shop.shop_table, shop.time, shop.last_updated";

	//フラグ指定
	$flag = $_GET["flag"];
	if(!isset($flag) || !is_numeric($flag)){
		$flag = "";
	}else{
		$flag = "(shop.flag & {$flag} > 0 OR shop.flag = {$flag})";
	}

	//SQL条件を生成
	$sql_where = "";
	if($shop_id === ""){
		if($span !== "") $sql_where .= "{$span} AND ";
		if($category !== "") $sql_where .= "{$category} AND ";
		if($flag !== "") $sql_where .= "{$flag} AND ";

		if(preg_match("/(.*) AND $/i", $sql_where, $match)) $sql_where = $match[1];
		if($sql_where !== ""){
			$sql_where = " WHERE ".$sql_where;
		}

		$groupby = " GROUP BY sales_history.shop_id";
	}else{
		$sql_where = " WHERE ".$shop_id." AND sales_history.prod_name IS NOT NULL";

		$groupby = " GROUP BY sales_history.prod_name";
		$sql_sel .= ", sales_history.prod_name";
	}

	//集計対象を指定
	$target = $_GET["target"];
	if(!isset($target)) throw new Exception("targetが指定されていません");

	switch($target){
		case "price":
			$sql_sel .= ", (SUM(sales_history.price)*SUM(sales_history.number)) AS rnk_sum";
			break;
		case "number":
			$sql_sel .= ", SUM(sales_history.number) AS rnk_sum";
			break;
		case "purenum":
			$sql_sel .= ", COUNT(sales_history.hist_id) AS rnk_sum";
			break;
		case "nicenum":
			$sql_sel .= ", IFNULL(nice_tmp.nice_num, 0) AS rnk_sum";
			break;
		default:
			throw new Exception("targetに正しくない値が指定されました");
			break;
	}

	//順位のオーダーを指定
	$order = $_GET["order"];
	if(!preg_match("/^(asc|desc)$/i", $order)){
		$order = " DESC";
	}
	$order = " ORDER BY rnk_sum {$order}";

	//順位の上限を指定
	$limit = $_GET["limit"];
	if(isset($limit) && is_numeric($limit)){
		if($limit > 1000){
			$limit = 1000;
		}

		$limit = " LIMIT {$limit}";
	}else{
		$limit = " LIMIT 10";
	}

	$sql_sel .= ", IFNULL(nice_tmp.nice, 'none') AS nice, IFNULL(nice_tmp.nice_num, 0) AS nice_num";

	//商品売上がない店舗を、ランキング対象に追加するSQL文
	$from_sales_text = "(SELECT * FROM sales_history UNION SELECT 0, NULL, NULL, 0, 0, shop.shop_id, 0 FROM shop LEFT JOIN (SELECT sales_history.shop_id FROM sales_history GROUP BY sales_history.shop_id)hist_tmp ON shop.shop_id=hist_tmp.shop_id WHERE hist_tmp.shop_id IS NULL)sales_history";

	$sql_str = "SELECT {$sql_sel}  FROM {$from_sales_text} INNER JOIN shop ON sales_history.shop_id=shop.shop_id INNER JOIN category ON shop.cat_id=category.cat_id LEFT JOIN (SELECT COUNT(nice_list.nice_id) AS nice_num, nice_list.shop_id, IF((SELECT sub_nice_list.nice_id FROM nice_list AS sub_nice_list WHERE sub_nice_list.shop_id=nice_list.shop_id AND sub_nice_list.user_id='{$esc_user_id}'), 'nice', 'none') AS nice FROM nice_list GROUP BY nice_list.shop_id) AS nice_tmp ON nice_tmp.shop_id=shop.shop_id{$sql_where}{$groupby}{$order}{$limit};";

	$sql_result = sql_query($dbcon, $sql_str);
	if(!$sql_result) throw new Exception("SQL問い合わせ失敗");

	$mode = ($shop_id===""?"shop":"product");

	$count = 0;
	print("<ranking>");
	while($sql_row = sql_fetch_assoc($sql_result)){
		print("<{$mode}>");
		print("<rank>".($count + 1)."</rank>");
		foreach($sql_row as $name => $value){
			$value = escapeXML($value);

			print("<{$name}>{$value}</{$name}>");
		}
		print("</{$mode}>");

		$count++;
	}
	sql_free_result($sql_result);
	print("</ranking>");

	print("<info>");
	print("<mode>{$mode}</mode>");
	print("<ranking_count>{$count}</ranking_count>");
	print("</info>");
	close_db($dbcon);
}catch(Exception $e){
	if($dbcon) close_db($dbcon);

	$status = $e->getMessage();
}

print("<status>{$status}</status>");
print("</data>");

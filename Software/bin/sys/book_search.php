<?php
session_start()
;
require_once("../../conf/conf.php");
require_once("../../lib/lib.php");
require_once("../../lib/dblib.php");

header("Content-Type: application/xml");
print('<?xml version="1.0" encoding="UTF-8"?>');
print("<data>");
$status = "ok";


try{
	if($_SESSION["book_login_status"] !== "logged in") throw new Exception("ログインしてください");
	$user_id = $_SESSION["user_id"];

	$dbcon = connect_db($CONNECT_DB_PARAM, "book_sys");
	if(!$dbcon) throw new Exception("DBの接続失敗");

	if(isset($_GET["q"])){
		$query = sql_escape_string($dbcon, $_GET["q"]);

		if(strlen($query) > 1000){
			throw new Exception("検索文は1000文字以内にしてください");
		}

		// スペース区切りのクエリワードを抽出
		$query = preg_split("/[ 　]+/u", $query, -1, PREG_SPLIT_NO_EMPTY);
		// 「id :     test」のようなクエリ文でも、正しく動作するよう処理
		$q_tmp = array();

		//クエリをSQLに変換
		for($index = 0; $index < array_count($query); $index++){
			if(preg_match("/(isbn|genre|title|auth|pub|desc|date)/ui", $query[$index])
				&& $query[$index + 1] === ':'
			){

				$q_tmp[] = $query[$index].$query[$index + 1].$query[$index + 2];

				$index += 2;
			}else{
				$q_tmp[] = $query[$index];
			}
		}
		$query = $q_tmp;

		$sql_where = "";
		if(array_count($query) > 0){
			$sql_where = "(";
		}
		for($query_index = 0; $query_index < array_count($query); $query_index++){
			$word = $query[$query_index];

			if($query_index + 1 < array_count($query) && !strcasecmp($word, "or")){
				if(preg_match("/^(.*) AND $/i", $sql_where, $tmp_match)){
					$sql_where = $tmp_match[1].")";

					$sql_where .= " OR (";
				}
			}else{
				if(!preg_match("/^(isbn|genre|title|auth|pub|desc|date):(.*)/ui", $word, $q_match)){
					$q_match[2] = $word;
				}

				$q_str_tbl = array("isbn" => "book_list.isbn", "genre" => "book_list.Genre", "title" => "book_list.Title", "auth" => "book_list.Author",
									"pub" => "book_list.Publisher", "desc" => "book_list.Description", "date" => "book_list.Date");

				$def_search_tbl = $q_str_tbl;
				unset($def_search_tbl["date"]);

				if(preg_match("/^([\\^\\$!])(.*)/iu", $q_match[2], $tmp_match)){
					if($tmp_match[1] === "^"){
						$lk_tmp = "'{$tmp_match[2]}%'";
					}else if($tmp_match[1] === '$'){
						$lk_tmp = "'%{$tmp_match[2]}'";
					}else if($tmp_match[1] === '!'){
						$lk_tmp = "'{$tmp_match[2]}'";
					}

					$orig_word = $tmp_match[2];
				}else{
					$lk_tmp = "'%{$q_match[2]}%'";

					$orig_word = $q_match[2];
				}

				switch($q_match[1]){
					case "isbn":
						$q_where_tmp = "book_list.isbn = '{$orig_word}'";
						break;
					case "genre":
					case "title":
					case "auth":
					case "pub":
					case "desc":
						$q_where_tmp = "{$q_str_tbl[$q_match[1]]} LIKE {$lk_tmp}";
						break;
					case "date":
						if(!preg_match("/(.*)-(.*)/u", $orig_word, $tmp_match)){
							$o_dates = array($orig_word);
						}else{
							$o_dates = array($tmp_match[1], $tmp_match[2]);
						}

						$date_status = TRUE;

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
									$split_dates[] = "{$tmp_match[1]}/{$tmp_match[2]}/{$tmp_match[3]}";
									$intv_str = "+ INTERVAL 1 DAY";
								}else if($arr_cnt > 2){
									if((int)$tmp_match[1] > 100){
										$split_dates[] = "{$tmp_match[1]}/{$tmp_match[2]}/01";
										$intv_str = "+ INTERVAL 1 MONTH";
									}else{
										$split_dates[] = gmdate('Y')."/{$tmp_match[1]}/{$tmp_match[2]}";
										$intv_str = "+ INTERVAL 1 DAY";
									}
								}else{
									$split_dates[] = "{$tmp_match[1]}/01/01";
									$intv_str = "+ INTERVAL 1 YEAR";
								}
							}else{
								$date_status = FALSE;
								break;
							}
						}

						if($date_status){
							$spd_index = 1;
							if($dates_count == 1) $spd_index = 0;

							$time_col_name = "book_list.Date";
							$q_where_tmp = "({$time_col_name} >= DATE('{$split_dates[0]}') AND {$time_col_name} < DATE('{$split_dates[$spd_index]}') {$intv_str})";
						}
						break;
					default:
						$q_where_tmp = "(";
						foreach($def_search_tbl as $q_str_tmp){
							$q_where_tmp .= $q_str_tmp." LIKE ".$lk_tmp." OR ";
						}
						preg_match("/^(.*) OR $/i", $q_where_tmp, $tmp_match);
						$q_where_tmp = $tmp_match[1].")";
						break;
				}
				$sql_where .= $q_where_tmp." AND ";
			}
		}
		if($sql_where !== ""){
			preg_match("/^(.*) AND $/i", $sql_where, $tmp_match);
			$sql_where = $tmp_match[1].")";
		}

		if($sql_where !== "") $sql_where = " WHERE {$sql_where}";


		//LIMITとOFFSETを設定
		$limit = $_GET["limit"];
		$offset = $_GET["offset"];
		if(!isset($limit) || !is_numeric($limit)) $limit = 10;
		if(!isset($offset) || !is_numeric($offset)) $offset = 0;

		$esc_user_id = sql_escape_string($dbcon, $user_id);
		$sql_str = "SELECT count(book_list.isbn) AS cnt FROM book_list{$sql_where};";

		$sql_result = sql_query($dbcon, $sql_str);
		if(!$sql_result) throw new Exception("shopの総件数取得失敗");
		$book_count = sql_fetch_assoc($sql_result)['cnt'];
		sql_free_result($sql_result);

		if($offset > $book_count) $offset = $book_count;
		if($limit > 1000) $limit = 1000;

		$sql_limit = " LIMIT {$limit} OFFSET {$offset}";

		$result_count = 0;
		if($book_count > 0){
			$order_tbl = array("isbn" => "book_list.isbn", "genre" => "book_list.Genre", "title" => "book_list.Title",
						"auth" => "book_list.Author", "pub" => "book_list.Publisher", "desc" => "book_list.Description",
						"date" => "book_list.Date", "bow" => "borrow_list.isbn", "srank" => "book_list.SalesRank",
						"price" => "book_list.price", "pages" => "book_list.NumberOfPages");

			$sql_order = "";
			if(isset($_GET["order"])){
				$order = $_GET["order"];

				$o_tmp = preg_split("/[ 　]/u", $order, -1, PREG_SPLIT_NO_EMPTY);
				if(!$o_tmp) $o_tmp[0] = $order;

				if(strlen($order_tbl[$o_tmp[0]]) > 0){
					if(preg_match("/^(asc|desc)$/i", $o_tmp[1])){
						$sql_order = " ORDER BY {$order_tbl[$o_tmp[0]]} {$o_tmp[1]}";
					}else{
						$sql_order = " ORDER BY {$order_tbl[$o_tmp[0]]} ASC";
					}
				}
			}

			//クエリに一致した図書一覧を返す
			$columns = "book_list.isbn, book_list.Author, book_list.Title, book_list.Publisher, book_list.Description, book_list.Genre, book_list.Date, book_list.SalesRank, book_list.Binding, book_list.Language, book_list.NumberOfPages, book_list.Price, IF(borrow_list.isbn IS NULL, 'exist', 'loan') AS book_status";
			$sql_str = "SELECT {$columns} FROM book_list LEFT JOIN borrow_list ON book_list.isbn=borrow_list.isbn{$sql_where}{$sql_order}{$sql_limit};";
			$sql_result = sql_query($dbcon, $sql_str);
			if(!$sql_result) throw new Exception($sql_str);

			print("<results>");

			while($sql_row = sql_fetch_assoc($sql_result)){
				print("<shop>");
				foreach($sql_row as $name => $value){
					$value = escapeXML($value);

					print("<{$name}>{$value}</{$name}>");
				}
				print("</shop>");

				$result_count++;
			}
			sql_free_result($sql_result);
			print("</results>");
		}

		print("<info>");
		print("<result_count>{$result_count}</result_count>");
		print("<limit>{$limit}</limit>");
		print("<current_offset>{$offset}</current_offset>");
		print("<book_count>{$book_count}</book_count>");
		print("</info>");
	}else{
		// クエリが未指定の場合
		$status = "クエリを指定してください。";
	}

	close_db($dbcon);

}catch(Exception $e){
	if($dbcon) close_db($dbcon);

	$status = $e->getMessage();
}
print("<status>{$status}</status>");

print("</data>");

?>
#!/usr/bin/php
<?php
if($argc < 2){
	die("ISBNを指定してください\n");
}else{
	$isbn = $argv[1];

	$isbn_ok = 0;
	//ISBN13のみ可
	if(strlen($isbn) == 13){
		//ISBNが正しいかチェック
		$sum = 0;
		for($i = 0; $i < strlen($isbn) - 1; $i++){
			if(!is_numeric($isbn[$i])){
				break;
			}

			$sum += (int)$isbn[$i] * ($i % 2 == 0 ? 1 : 3);
		}

		$digit = $sum % 10;
		$digit = 10 - $digit;
		if($digit == 10) $digit = 0;

		if($digit == (int)$isbn[strlen($isbn) - 1]){
			$isbn_ok = 1;
		}
	}

	if(!$isbn_ok){
		die("ISBNが正しくありません\n");
	}
}


$access_key_id = '<<INPUT_ACCESS_KEY_ID>>';
$secret_access_key = '<<INPUT_SECRET_ACCESS_KEY>>';

$baseurl = 'http://ecs.amazonaws.jp/onca/xml';
$params = array();
$params['Service']        = 'AWSECommerceService';
$params['AWSAccessKeyId'] = $access_key_id;
$params['AssociateTag'] = 'booksys-22';
$params['Operation']      = 'ItemLookup';
$params['SearchIndex']    = 'Books';
$params['ResponseGroup']    = 'Large';
$params['IdType']    = 'ISBN';
$params['ItemId']       = $isbn;
$params['Timestamp'] = gmdate('Y-m-d\TH:i:s\Z');

ksort($params);

$str_array = array();
foreach ($params as $key => $value) {
    $str_array[] = rawurlencode($key)."=".rawurlencode($value);
}

$str_params = implode("&", $str_array);
$parsed_url = parse_url($baseurl);
$string_to_sign = "GET\n{$parsed_url['host']}\n{$parsed_url['path']}\n".$str_params;
$signature = base64_encode(hash_hmac("sha256", $string_to_sign, $secret_access_key, true));

$url = $baseurl."?".$str_params."&Signature=".rawurlencode($signature);
$book_info = array();

$res_xml = @simplexml_load_file($url);
if($res_xml !== FALSE){
	$book_info["description"] = "なし";
	$item_kindle = NULL;
	$item_list = array();
	if(isset($res_xml->Items)){
		foreach($res_xml->Items->children() as $item){
			if(isset($item->ItemAttributes)){
				$attr = $item->ItemAttributes;

				$binding = (string)$attr->Binding;

				if(preg_match("/kindle/si", $binding)){
					$item_kindle = $item;
				}else{
					$item_list[] = $item;
				}

				//EditorialReviewを見つける
				if(isset($item->EditorialReviews)){
					$review = (string)$item->EditorialReviews->EditorialReview[0]->Content;

					$book_info["description"] = $review;
				}
			}
		}

		if($item_kindle !== NULL){
			$item_list[] = $item_kindle;
		}
	}

	if(count($item_list) > 0){
		foreach($item_list as $item_node){
			//売り上げランクを取得
			if(isset($item_node->SalesRank)){
				if(!isset($book_info["salesrank"])) $book_info["salesrank"] = $item_node->SalesRank;
			}

			//図書カバー画像を取得
			foreach($item_node->children() as $key => $value){
				$key = strtolower($key);
				$value = (string)$value->URL;
				switch($key){
					case "smallimage":
						if(!isset($book_info["s_image"])) $book_info["s_image"] = $value;
					break;
					case "mediumimage":
						if(!isset($book_info["m_image"])) $book_info["m_image"] = $value;
					break;
					case "largeimage":
						if(!isset($book_info["l_image"])) $book_info["l_image"] = $value;
					break;
				}
			}

			$author_tmp = array();
			//図書情報を取得
			foreach($item_node->ItemAttributes->children() as $key => $value){
				$key = strtolower($key);

				switch($key){
					case "author":
					case "creator":
						$author_tmp[] = (string)$value;
					break;
					case "publisher":
						if(!isset($book_info["publisher"])) $book_info["publisher"] = (string)$value;
					break;
					case "title":
						if(!isset($book_info["title"])) $book_info["title"] = (string)$value;
					break;
					case "releasedate":
					case "publicationdate":
						if(!isset($book_info["date"])) $book_info["date"] = date("Y-m-d H:i:s", strtotime((string)$value));
					break;
					case "binding":
						if(!isset($book_info["binding"])) $book_info["binding"] = (string)$value;
					break;
					case "numberofpages":
						if(!isset($book_info["numberofpages"])) $book_info["numberofpages"] = (string)$value;
					break;
					case "listprice":
						if(!isset($book_info["price"])) $book_info["price"] = $value->FormattedPrice;
					break;
					case "languages":
						if(!isset($book_info["language"])) $book_info["language"] = $value->Language[0]->Name;
					break;
				}
			}
			if(!isset($book_info["author"])){
				$book_info["author"] = $author_tmp;
			}

			//ジャンルを抜き出す処理
			$genre_name = "なし";
			$genre_array = array();
			foreach($item_node->BrowseNodes->BrowseNode as $bnode){
				$node_func = function($node) use (&$node_func, &$genre_array){
					if(isset($node->Ancestors)){
						$node_func($node->Ancestors->BrowseNode);
					}

					$genre_array[] = (string)$node->Name;
				};

				$genre_array = array();
				$node_func($bnode);
				if(preg_match("/洋書/u", $genre_array[0])){
					$genre_name = "洋書";
					break;
				}else if(preg_match("/本/u", $genre_array[0])){
					if(preg_match("/ジャンル別/u", $genre_array[1])){
						if(isset($genre_array[2])) $genre_name = $genre_array[2];
						break;
					}
				}
			}
			if(!isset($book_info["genre"])) $book_info["genre"] = $genre_name;
		}

		//l_image,m_image,s_imageの順で探してあれば、画像をダウンロードしBASE64で返す
		$image_url_list = array($book_info["l_image"], $book_info["m_image"], $book_info["s_image"]);
		foreach($image_url_list as $url){
			if(isset($url)){
				$image_dat = file_get_contents($url);
				if($image_dat !== FALSE){
					$book_info["image_url"] = $url;

					//下で改行を入れる処理を入れているため改行抜きで入れる
					$book_info["image_data"] = preg_replace("/[\r\n]/s", "", base64_encode($image_dat));
					break;
				}
			}
		}

		print("SUCCESS\n");
		foreach($book_info as $key => $value){
			if($key === "image_data") continue; //image_dataは後で出力

			if(is_array($value)){
				$value = implode(", ", $value);
			}
			print($key.":".$value."\n");
		}

		//base64があれば出力
		if(isset($book_info["image_data"])){
			//base64_encodeは76文字ごとに改行を入れないためその対策
			$base64_tmp = $book_info["image_data"];
			while(strlen($base64_tmp) > 76) {
				print("image_data:" . substr($base64_tmp, 0, 76) . "\n");

				$base64_tmp = substr($base64_tmp, 76, strlen($base64_tmp) - 76);
			};
			print("image_data:" . $base64_tmp . "\n");
		}
	}else{
		die("書籍情報XMLの構造が正しくありません\n");
	}
}else{
	die("書籍情報問合せに失敗しました\n");
}

?>
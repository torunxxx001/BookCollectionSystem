<?php
//汎用ライブラリ

// 配列の要素数をカウント
// 配列ではない場合は0を返す
function array_count($input)
{
	return is_array($input) ? count($input) : 0;
}

//XML用に文字列をエスケープする
function escapeXML($string)
{
	$string = htmlspecialchars($string);
	$string = preg_replace("/&(.+?;)/s", "&amp;$1", $string);

	return $string;
}

//与えられた配列が連想配列かチェック
function is_hash(&$in_array) {
    $index = 0;
    foreach($in_array as $key => $value) {
        if ( $key !== $index++ ) return TRUE;
    }
    return FALSE;
}


?>
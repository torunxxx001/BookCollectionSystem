<?php

//与えられたユーザＩＤが管理ユーザか判断
// ret = TRUE:管理ユーザ, FALSE:違う
function check_admin($dbcon, $user_id)
{
	$result = FALSE;

	$esc_user_id = sql_escape_string($dbcon, $user_id);
	$sql_result = sql_query($dbcon, "SELECT * FROM book_sys.user_list WHERE user_id = '{$esc_user_id}' AND admin = TRUE;");
	if(!$sql_result) throw new Exception("管理ユーザ問合せ失敗");

	if(sql_num_rows($sql_result) > 0){
		$result = TRUE;
	}
	sql_free_result($sql_result);

	return $result;
}

?>
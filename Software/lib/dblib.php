<?php
//データベース用ライブラリ


// MySQLの関数を別名定義

function connect_db($config_param, $db_name)
{
	$con = new mysqli($config_param["host"], $config_param["user"], $config_param["password"], $db_name);

	$con->set_charset("utf8");

	return $con;
}

function sql_query($con, $sql)
{
	return $con->query($sql);
}

// マージ関数
// UPDATEしてマッチ数が０の場合はINSERT実行
function sql_merge($con, $insert_sql, $update_sql)
{
	$sql_result = sql_query($con, $update_sql);

	if($sql_result){
		$info_str = mysql_info($con);
		preg_match("/matched.*?(\d)+/i", $info_str, $match);

		if((int)$match[1] == 0){
			$sql_result = sql_query($con, $insert_sql);
		}
	}

	return $sql_result;
}

function sql_select_db($con, $db_name)
{
	return $con->select_db($db_name);
}

function sql_prepare($con, $sql)
{
	return $con->prepare($sql);
}

function sql_error($con)
{
	return $con->error;
}

function sql_affected_rows($con)
{
	return $con->affected_rows;
}

function sql_num_rows($resource)
{
	return $resource->num_rows;
}

function sql_fetch_assoc($resource)
{
	return $resource->fetch_assoc();
}

function sql_fetch_all($resource)
{
	$result = array();
	while($row = sql_fetch_assoc($resource)){
		$result[] = $row;
	}

	return $result;
}

function close_db($con)
{
	return $con->close();
}

function sql_free_result($result_obj)
{
	$result_obj->free_result();
}

function sql_escape_string($con, $string)
{
	return $con->real_escape_string($string);
}

function sql_stmt_execute($stmt)
{
	return $stmt->execute();
}

function sql_stmt_fetch($stmt)
{
	return $stmt->fetch();
}

function sql_stmt_store_result($stmt)
{
	return $stmt->store_result();
}

function sql_stmt_free_result($stmt)
{
	return $stmt->free_result();
}

function sql_stmt_close($stmt)
{
	return $stmt->close();
}


function sql_stmt_bind_param($stmt, $type, &$param_array)
{
	$param_array = array();
	$args = array();

	$args[0] = $type;
	for($index=0; $index < strlen($type); $index++){
		$param_array[$index] = NULL;
		$args[] = &$param_array[$index];
	}

	return call_user_func_array([$stmt, "bind_param"], $args);
}

function sql_stmt_bind_result($stmt, &$result_array)
{
	$result_array = array();

	$meta = $stmt->result_metadata();
	$args = array();
	while($f_name = $meta->fetch_field()){
		$result_array[$f_name->name] = NULL;
		$args[] = &$result_array[$f_name->name];
	}

	return call_user_func_array([$stmt, "bind_result"], $args);
}

?>
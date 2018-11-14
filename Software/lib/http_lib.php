<?php
//HTTP通信用ライブラリ

class HttpClass
{
	public $lastResponseHeaders = array();
	private $send_headers = "";

	//ヘッダを連想配列に変換するメソッド
	public function ConvertHeaders($array_headers)
	{
		$cnt = -1;
		$headers = array();
		foreach($array_headers as $header){
			if(preg_match("/^HTTP\\/1.*$/i", $header)){
				$cnt++;
				$headers[$cnt]["status"] = $header;
			}else{
				preg_match("/^(.*?) *: *(.*?)$/i", $header, $matches);
				$headers[$cnt][strtolower($matches[1])] = $matches[2];
			}
		}
		$headers["count"] = $cnt;

		return $headers;
	}

	/* ヘッダを設定するメソッド
	 	パラメタ
	 	$array_headers = array(
	 		"Content-Type" => "text/html"
			...
	 	);
	*/
	public function SetHeaders($array_headers)
	{
		$this->send_headers = "";
		if(is_array($array_headers)){
			foreach($array_headers as $header_name => $header_value){
				$this->send_headers .= "$header_name: $header_value\r\n";
			}
		}
	}

	//通信用コンテキストを作成するメソッド
	public function MakeContext($method = "GET", $params = NULL)
	{
		$context = array("http" => array());
		$context["http"]["method"] = $method;

		if($send_headers !== ""){
			$context["http"]["header"] = $this->send_headers;
		}
		if($params !== NULL){
			if(preg_match("/^POST$/i", $method) && isset($params["post_data"])) 
				$context["http"]["content"] = $params["post_data"];
		}

		return $context;
	}

	//HTTPのヘッダのみを取得するメソッド
	public function GetHeaders($url, $method = "GET", $params = NULL)
	{
		stream_context_set_default($this->MakeContext($method, $params));
		return $this->ConvertHeaders(get_headers($url));
	}

	//HTTP通信するメソッド
	/*	パラメタ
		$params = array(
			"post_data" => array(
				"name" => "value"
			)
		);
	*/
	public function HttpAction($url, $method = "GET", $params = NULL)
	{
		$response_data = file_get_contents($url, false, stream_context_create($this->MakeContext($method, $params)));

		$this->lastResponseHeaders = $this->ConvertHeaders($http_response_header);

		return $response_data;
	}
}

?>

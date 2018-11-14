<?php
//HTTP�ʐM�p���C�u����

class HttpClass
{
	public $lastResponseHeaders = array();
	private $send_headers = "";

	//�w�b�_��A�z�z��ɕϊ����郁�\�b�h
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

	/* �w�b�_��ݒ肷�郁�\�b�h
	 	�p�����^
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

	//�ʐM�p�R���e�L�X�g���쐬���郁�\�b�h
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

	//HTTP�̃w�b�_�݂̂��擾���郁�\�b�h
	public function GetHeaders($url, $method = "GET", $params = NULL)
	{
		stream_context_set_default($this->MakeContext($method, $params));
		return $this->ConvertHeaders(get_headers($url));
	}

	//HTTP�ʐM���郁�\�b�h
	/*	�p�����^
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

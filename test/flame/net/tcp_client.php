<?php
flame\init("tcp_client_test");
flame\go(function() {
	$sock = new flame\net\tcp_socket();
	yield $sock->connect("127.0.0.2", 7678);
	var_dump($sock);
	while(true) {
		$data = yield $sock->read(2);
		echo "=> ", $data, "\n";
		if($data === null) { // 连接被对方关闭（EOF）
			break;
		}
	}
});
flame\run();

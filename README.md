## Flame
**Flame** 是一个 PHP 框架，借用 PHP Generator 实现 协程式 的编程服务。

## 文档
https://terrywh.github.io/php-flame/


## 示例
``` PHP
<?php
// 框架初始化（自动设置进程名称）
flame\init("http-server", [
	"worker" => 4, // 多进程服务
]);
// 启用一个协程作为入口
flame\go(function() {
	// 创建 http 处理器
	$handler = new flame\net\http\handler();
	// 设置默认处理程序
	$handler->handle(function($req, $res) {
		yield $res->write_header(404);
		yield flame\time\sleep(2000);
		yield $res->end("not found");
	})->get("/hello", function($req, $res) {
		yield $res->end("hello world");
	});
	// 创建网络服务器（这里使用 TCP 服务器）
	$server = new flame\net\tcp_server();
	// 指定处理程序
	$server->handle($handler);
	// 绑定地址（支持 IPv6）
	$server->bind("::", 19001);
	yield $server->run();
});
// 框架调度执行
flame\run();
```

## 额外依赖

### 编译安装

为了使用一些较新功能 (例如 HTTP2 协议支持), FLAME 内置了很躲依赖库较新的版本; 但为防止与一些系统库冲突, 部分库需要额外安装或更新:

* 更新或安装系统库
```
yum install cyrus-sasl-devel openssl-devel 
yum update
```

* https://github.com/c-ares/c-ares
```
./configure --prefix=/usr --libdir=/usr/lib64
make
make install
```
* https://github.com/nghttp2/nghttp2
```
autoreconf -i
automake
autoconf
./configure --prefix=/usr --libdir=/usr/lib64
make 
make install
```
* https://github.com/curl/curl
```
./buildconf
PKG_CONFIG_PATH=/usr/lib64/pkgconfig ./configure --prefix=/usr --libdir=/usr/lib64 --with-nghttp2 --enable-ares
make
make install
```

### 注意

* 若复制使用上述库, 请注意在系统路径中上述软链有可能没有自动更改;

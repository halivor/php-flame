## Flame
**Flame** 是一个 PHP 框架，借用 PHP Generator 实现 协程式 的编程 (协程即调度执行的线程内的独立“执行单位”，可以认为互相独立运行). 目前，flame 中提供了如下功能：
1. 核心功能
	1. [核心](/php-flame/core) - 初始化, 协程启动;
	2. [时间](/php-flame/time) - 毫秒时间戳, 调度休眠、定时器;
	3. [系统](/php-flame/os) - 网卡信息, 异步进程;
	4. [日志](/php-flame/log) - 简单日志记录 API，日志重载信号；
2. 网络
	1. [TCP](/php-flame/tcp) - 客户端, 服务器;
	2. [HTTP](/php-flame/http) - 客户端, 服务器;
	5. [UDP](/php-flame/udp) - 客户端, 服务器;
3. 驱动
	1. [Redis](/php-flame/redis) - 简单客户端；
	2. [MySQL](/php-flame/mysql) - 简单客户端;
	3. [RabbitMQ](/php-flame/rabbitmq) - 简单客户端；
	4. [Kafka](/php-flame/kafka) - 简单客户端；
	5. [Mongodb](/php-flame/mongodb) - 简单客户端;

## 示例
``` PHP
<?php
// 框架初始化（自动设置进程名称）
flame\init("http-server", [
	"worker" => 4, // 多进程服务
]);
// 启用一个协程作为入口
flame\go(function() {
	// 创建 http 服务器
	$server = new flame\http\server(":::19001");
	// 设置处理过程
	$server->before(function($req, $res, $r) {
		if(!$r) {
			yield $res->write_header(404);
			yield flame\time\sleep(2000);
			yield $res->end("not found");
		}
	})->get("/hello", function($req, $res) {
		$res->body = "hello, world!\n";
	});
	// 运行服务
	yield $server->run();
});
// 框架调度执行
flame\run();
```

## 仓库
[https://github.com/terrywh/php-flame/](https://github.com/terrywh/php-flame/)

## 使用
* 程序入口大致固定：
```
<?php
// 框架初始化：
flame\init("进程名称");
flame\go(function() {
/*首个协程, 逻辑从这里开始*/
});
// 框架主流程调度：
flame\run();
```
* 本文档函数说明前置 `yield` 关键字, 标识此函数为 "**异步函数**";
* 调用异步函数需要使用 `yield fn(...);` 形式；
* 存在异步函数调用的**封装函数**也是异步函数 (即：需要 `yield fn(...);` 形式调用)；
* 构造、析构函数等特殊函数无法定义为异步函数 (即：无法在构造或析构函数中调用异步函数);

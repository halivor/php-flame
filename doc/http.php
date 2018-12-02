<?php

namespace flame\http;

function exec(client_request $req): client_response {}
function get(string $url, int $timeout = 3000): client_response {}
function post(string $url, mixed $data, int $timeout = 3000): client_response {}
function put(string $url, mixed $data, int $timeout = 3000): client_response {}
function delete(string $url, int $timeout = 3000): client_response {}

class client_request {
    /**
     * @property int
     */
    public $timeout;
    /**
     * @property string
     */
    public $method;
    /**
     * @property string
     */
    public $url;
    /**
     * @property array
     */
    public $header;
    /**
     * @property array
     */
    public $cookie;
    /**
     * @property mixed
     */
    public $body;
    /**
     * 构建请求, 可选的指定请求体 `$body` (将自动设置为 POST 请求方法)及超时时间 `$timeout`
     */
    function __construct(string $url, mixed $body = null, int $timeout = 3000) {}
}

class client_response {
    /**
     * 响应码
     * @property int
     */
    public $status;
    /**
     * 注意: 目前使用了数组形式, 切暂不支持多个同名 Header 数据的访问;
     * @property array
     */
    public $header;
    /**
     * @property mixed 以下几种 Content-Type 将自动进行解析并生成关联数组:
     *  * "application/json"
     *  * "application/x-www-form-urlencoded"
     *  * "multipart/form-data"
     * 其他类型保持文本的原始数据(与 $rawBody 相同);
     */
    public $body;
    /**
     * 原始请求体数据
     * @property string
     */
    public $rawBody;
}

class server {
    /**
     * 服务端本地监听地址
     */
    public $address;
    /**
     * 创建服务器并绑定地址
     */
    function __construct(string $address) {}
    /**
     * 每次 HTTP 请求在执行对应 path 的处理器前, 先执行下述设置的回调;
     * 回调形式如下:
     *  function callback(server_request $req, server_response $res, bool $match) {}
     * 其中 $match 参数标识此请求是否匹配了定义的 path 处理器;
     * 当 callback 函数返回 false 时, 将**不再**执行后续处理器
     */
    function before(callable $cb):server {}
    /**
     * 每次 HTTP 请求在执行对应 path 处理器后, 会执行下述设置的回调;
     * 回调形式同 before();
     * 注意: 若 before 回调或 path 回调返回了 false, 此 after 回调将不再执行;
     */
    function after(callable $cb):server {}
    /**
     * 设置一个处理 "PUT $path HTTP/1.1" 请求的路径 path 处理器;
     * 注意: 当 before 回调返回 false 时, 此处理器将不再被执行;
     */
    function put(string $path, callable $cb):server {}
    /**
     * 设置一个处理 "DELETE $path HTTP/1.1" 请求的路径 path 处理器;
     * 注意: 当 before 回调返回 false 时, 此处理器将不再被执行;
     */
    function delete(string $path, callable $cb):server {}
    /**
     * 设置一个处理 "POST $path HTTP/1.1" 请求的路径 path 处理器;
     * 注意: 当 before 回调返回 false 时, 此处理器将不再被执行;
     */
    function post(string $path, callable $cb):server {}
    /**
     * 设置一个处理 "PATCH $path HTTP/1.1" 请求的路径 path 处理器;
     * 注意: 当 before 回调返回 false 时, 此处理器将不再被执行;
     */
    function patch(string $path, callable $cb):server {}
    /**
     * 设置一个处理 "GET $path HTTP/1.1" 请求的路径 path 处理器;
     * 注意: 当 before 回调返回 false 时, 此处理器将不再被执行;
     */
    function get(string $path, callable $cb):server {}
    /**
     * 设置一个处理 "HEAD $path HTTP/1.1" 请求的路径 path 处理器;
     * 注意: 当 before 回调返回 false 时, 此处理器将不再被执行;
     */
    function head(string $path, callable $cb):server {}
    /**
     * 设置一个处理 "OPTIONS $path HTTP/1.1" 请求的路径 path 处理器;
     * 注意: 当 before 回调返回 false 时, 此处理器将不再被执行;
     */
    function options(string $path, callable $cb):server {}
    
    /**
     * 启动服务器, 监听请求并执行对应回调
     * 注意: 运行服务器将阻塞当前协程;
     */
    function run() {}
    /**
     * 停止服务器, 不再接收新的连接; 上述 run() 阻塞协程将恢复执行;
     * 注意: 正在处理中的请求不收影响(继续处理);
     */
    function close() {}
}

class server_request {
    /**
     * 请求方法
     * @property string
     */
    public $method = "GET";
    /**
     * 请求路径
     * @property string
     */
    public $path = "/";
    /**
     * 请求参数, 使用 PHP 内置 parse_str() 函数解析得到;
     * @property array
     */
    public $query;
    /**
     * 请求头; 所有头信息字段名被转换为**小写**形式;
     * 暂不支持多项同名 Header 字段;
     * @property array
     */
    public $header;
    /**
     * 请求 Cookie
     * @property array
     */
    public $cookie;
    /**
     * 请求体, 以下几种 Content-Type 时自动解析为关联数组:
     *  * "application/json"
     *  * "application/x-www-form-urlencoded"
     *  * "multipart/form-data"
     * 其他类型时与 $rawBody 相同;
     * @property mixed
     */
    public $body;
    /**
     * 原始请求体
     * @property string
     */
    public $rawBody;
    /**
     * 用户可设置的请求关联数据;
     * 例如: 在 before 回调中从 Cookie 中提取用户信息放在 $req->data["user"] 中, 后续处理器即可访问 $req->data["user"] 获取户信息;
     * @property array
     */
    public $data;
}

/**
 * 服务端响应对象, 提供接口向客户端返回数据
 */
class server_response {
    /**
     * 响应状态码, 可以直接设置; 也可以在 Transfer-Encoding: chunked 模式时使用 write_header 指定;
     * @property int
     */
    public $status = 200;
    /**
     * 响应头信息, Key/Val 形式
     * @property array
     */
    public $header;
    // public $cookie;
    /**
     * 响应体, 用于在 Content-Length: xxx 响应模式下返回数据;
     * @property mixed 任意数据, 实际响应会序列化此数据; 以下几种 Content-Type 存在内置的序列化:
     *  * "application/json" - 使用 json_encode 进行序列化;
     *  * "application/x-www-form-urlencoded" - 使用 http_build_query 进行序列化;
     * 其他类型强制转换为文本类型后响应;
     */
    public $body;
    /**
     * 设置返回的 Set-Cookie 项;
     * 参数功能与 PHP 内置 set_cookie 函数类似;
     */
    function set_cookie(string $name, string $value = null, int $expire = 0, string $path = "/", string $domain = "", bool $secure = false, bool $http_only = false) {}
    /**
     * 返回响应头, 用于启用 Transfer-Encoding: chunked 模式, 准备持续响应数据;
     * @param int $status 当参数存在时, 覆盖上述 $res->status 属性响应码;
     * 
     * Transfer-Encoding: chunked 模式可用于 EventSource 等持续响应;
     */
    function write_header($status = 0) {}
    /**
     * 在 Transfer-Encoding: chunked 模式下, 返回一个 chunk 数据块;
     */
    function write(string $chunk) {}
    /**
     * 在 Transfer-Encoding: chunked 模式下, 结束响应过程 (返回最后一个结束块);
     * @param string $chunk 可选, 调用上述 write() 返回一个数据块;
     */
    function end(string $chunk = null) {}
    /**
     * 响应一个文件的内容; 一般需要自行设置对应文件的 Content-Type 以保证浏览器能正常渲染;
     * 下述参数中的 $path 将会被正常化处理, 以保证其不超出 $root 指定的根目录范围;
     * 注意: 此功能一般仅用于调试或少量文件访问使用, 大量生产环境请考虑使用 nginx 代为处理静态文件;
     */
    function file(string $root, string $path) {}
}
#pragma once

namespace flame {
namespace http {
	class handler;
	class server_response: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		// 声明为 ZEND_ACC_PRIVATE 禁止创建（不会被调用）
		php::value __construct(php::parameters& params);
		php::value __destruct(php::parameters& params);
		php::value set_cookie(php::parameters& params);
		php::value write_header(php::parameters& params);
		php::value write(php::parameters& params);
		php::value end(php::parameters& params);
		php::value file(php::parameters& params);
		server_response();
	private:
		int status_;
		std::shared_ptr<handler> handler_; // 允许通过 $res 对象保留 handler 持续响应
		void build_ex(boost::beast::http::message<false, value_body<false>>& ctr);

		friend class handler;
	};
}
}
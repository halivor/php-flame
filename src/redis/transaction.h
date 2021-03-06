#pragma once

namespace flame {
namespace redis {
	class _transaction;
	class transaction: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params); // 私有
		php::value __destruct(php::parameters& params); // 私有
		php::value __call(php::parameters& params);
		// 处理特殊情况的命令
		php::value mget(php::parameters& params);
		php::value hmget(php::parameters& params);
		php::value hgetall(php::parameters& params);
		php::value hscan(php::parameters& params);
		php::value sscan(php::parameters& params);
		php::value zscan(php::parameters& params);
		php::value zrange(php::parameters& params);
		php::value zrevrange(php::parameters& params);
		php::value zrangebyscore(php::parameters& params);
		php::value zrevrangebyscore(php::parameters& params);
		php::value unsubscribe(php::parameters& params);
		php::value punsubscribe(php::parameters& params);
		php::value exec(php::parameters& params);
		// 用于标记不支持的功能
		php::value unsupport(php::parameters& params);
	private:
		// 实际的客户端对象可能超过当前对象的生存期
		std::shared_ptr<_client> cli_;
		_transaction*             tx_;

		friend class client;
	};
}
}
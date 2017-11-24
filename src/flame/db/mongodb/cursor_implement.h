#pragma once

namespace flame {
	class thread_worker;
namespace db {
namespace mongodb {
	class cursor;
	class cursor_implement {
	public:
		cursor_implement(std::shared_ptr<thread_worker> worker, cursor* c, mongoc_cursor_t* cs);
	private:
		std::shared_ptr<thread_worker> worker_;
		cursor*                        cpp_;
		mongoc_cursor_t*               css_;
		
		static void     next_wk(uv_work_t* req);
		static void to_array_wk(uv_work_t* req);
		static void    close_wk(uv_work_t* req);
		friend class cursor;
	};
	
	typedef struct cursor_request_t {
		coroutine*          co;
		cursor_implement* self;
		php::value          rv; // 引用 + 返回
		uv_work_t          req;
	} cursor_request_t;
}
}
}
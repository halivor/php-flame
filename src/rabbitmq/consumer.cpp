#include "../coroutine.h"
#include "consumer.h"
#include "_client.h"
#include "message.h"
#include "../time/time.h"

namespace flame::rabbitmq
{
    void consumer::declare(php::extension_entry &ext)
    {
        php::class_entry<consumer> class_consumer("flame\\rabbitmq\\consumer");
        class_consumer
            .method<&consumer::__construct>("__construct", {}, php::PRIVATE)
            .method<&consumer::__destruct>("__destruct")
            .method<&consumer::run>("run",
            {
                {"callable", php::TYPE::CALLABLE},
            })
            .method<&consumer::confirm>("confirm",
            {
                {"message", "flame\\rabbitmq\\message"}
            })
            .method<&consumer::reject>("reject",
            {
                {"message", "flame\\rabbitmq\\message"},
                {"requeue", php::TYPE::BOOLEAN, false, true},
            })
            .method<&consumer::close>("close");
        ext.add(std::move(class_consumer));
    }
    php::value consumer::__construct(php::parameters &params)
    {
        return nullptr;
    }
    php::value consumer::__destruct(php::parameters& params)
    {
        return nullptr;
    }
    php::value consumer::run(php::parameters &params)
    {
        cb_ = params[0];
        
        coroutine_handler ch {coroutine::current};
        // 下述 q 需要在堆分配, 否则当 cc_->consume() 结束后可能与被提前销毁 (协程销毁稍晚)
        coroutine_queue<php::object> q(cc_->pf_);
        // 启动若干协程, 然后进行"并行>"消费
        int count = (int)std::ceil(cc_->pf_ / 2.0);
        for (int i = 0; i < count; ++i)
        {
            // 启动协程开始消费
            coroutine::start(php::value([this, &q, &count, &ch, i] (php::parameters &params) -> php::value {
                coroutine_handler cch {coroutine::current};
                while(auto x = q.pop(cch))
                {
                    try
                    {
                        cb_.call( {x.value()} );
                    }
                    catch(const php::exception& ex)
                    {
                        std::clog << "[" << time::iso() << "] (ERROR) " << ex.what() << std::endl;
                    }
                }
                if(--count == 0)
                {
                    ch.resume();
                }
                return nullptr;
            }));
        }
        cc_->consume(cq_, q, ch);
        ch.suspend();
        cb_ = nullptr;
        return nullptr;
    }
    php::value consumer::confirm(php::parameters &params)
    {   
        php::object obj = params[0];
        message *ptr = static_cast<message *>(php::native(obj));
        coroutine_handler ch {coroutine::current};
        cc_->confirm(ptr->tag_, ch);
        return nullptr;
    }
    php::value consumer::reject(php::parameters &params)
    {
        php::object obj = params[0];
        message *ptr = static_cast<message *>(php::native(obj));
        int flags = 0;
        if (params.size() > 1 && params[1].to_boolean()) flags |= AMQP::requeue;
        coroutine_handler ch {coroutine::current};
        cc_->reject(ptr->tag_, flags, ch);
        return nullptr;
    }
    php::value consumer::close(php::parameters &params)
    {
        coroutine_handler ch {coroutine::current};
        cc_->close_consumer(ch);
        return nullptr;
    }

} // namespace flame::rabbitmq

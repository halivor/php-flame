#include "controller.h"
#include "coroutine.h"
#include "core.h"
#include "coroutine_queue.h"
#include "queue.h"
#include "mutex.h"

namespace flame::core {
    php::value select(php::parameters& params)
    {
        std::vector< std::shared_ptr<coroutine_queue<php::value>> > qs;
        std::map< std::shared_ptr<coroutine_queue<php::value>>, php::object > mm;
        for (auto i = 0; i < params.size(); ++i)
        {
            if(!params[i].instanceof(php::class_entry<queue>::entry()))
            {
                throw php::exception(zend_ce_type_error, "only flame\\queue can be selected", -1);
            }
            php::object obj = params[i];
            queue* ptr = static_cast<queue*>(php::native(obj));
            qs.push_back( ptr->q_ );
            mm.insert({ptr->q_, obj});
        }
        coroutine_handler ch{coroutine::current};
        std::shared_ptr<coroutine_queue<php::value>> q = select_queue(qs, ch);
        return mm[q];
    }
    void declare(php::extension_entry &ext) {
        ext
            .function<select>("flame\\select");

        queue::declare(ext);
        mutex::declare(ext);
        guard::declare(ext);
    }
    
}
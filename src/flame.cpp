#include "vendor.h"
#include "controller.h"
#include "controller_master.h"
#include "controller_worker.h"
#include "coroutine.h"
#include "core.h"
#include "os/os.h"
#include "time/time.h"
#include "mysql/mysql.h"
#include "redis/redis.h"
#include "mongodb/mongodb.h"
#include "kafka/kafka.h"
#include "rabbitmq/rabbitmq.h"
#include "tcp/tcp.h"

namespace flame 
{
    static php::value init(php::parameters& params)
    {
        php::array options(0);
        if(params.size() > 1 && params[1].typeof(php::TYPE::ARRAY))
        {
            options = params[1];
        }
        gcontroller->core_execute_data = EG(current_execute_data);
        gcontroller->initialize(params[0], options);
        return nullptr;
    }
    static php::value go(php::parameters& params)
    {
        php::callable fn = params[0];
        coroutine::start(fn);
        return nullptr;
    }
    static php::value fake_go(php::parameters& params)
    {
        return nullptr;
    }
    static php::value run(php::parameters& params) {
        if(gcontroller->status & controller::STATUS_INITIALIZED)
        {
            gcontroller->core_execute_data = EG(current_execute_data);
            gcontroller->run();
        }else{
            throw php::exception(zend_ce_type_error, "failed to run flame: not yet initialized (forget to call 'flame\\init()' ?)");
        }
        return nullptr;
    }
}

extern "C"
{
    ZEND_DLEXPORT zend_module_entry *get_module()
    {
        static php::extension_entry ext(EXTENSION_NAME, EXTENSION_VERSION);
        std::string sapi = php::constant("PHP_SAPI");
        if (sapi != "cli")
        {
            std::cerr << "Flame can only be using in SAPI='cli' mode\n";
            return ext;
        }

        php::class_entry<php::closure> class_closure("flame\\closure");
        class_closure.method<&php::closure::__invoke>("__invoke");
        ext.add(std::move(class_closure));

        ext
            .desc({"vendor/boost", BOOST_LIB_VERSION})
            .desc({"vendor/libphpext", PHPEXT_LIB_VERSION})
            .desc({"vendor/amqpcpp", "4.0.0"})
            .desc({"vendor/mysqlc", PACKAGE_VERSION})
            .desc({"vendor/librdkafka", rd_kafka_version_str()})
            .desc({"vendor/mongoc", MONGOC_VERSION_S});

        flame::gcontroller.reset(new flame::controller());
        ext
            .function<flame::init>("flame\\init",
            {
                {"process_name", php::TYPE::STRING},
                {"options", php::TYPE::ARRAY, false, true},
            })
            .function<flame::run>("flame\\run");
        if(flame::gcontroller->type == flame::controller::process_type::WORKER)
        {
            ext.function<flame::go>("flame\\go",
            {
                {"coroutine", php::TYPE::CALLABLE},
            });
            flame::core::declare(ext);
            flame::os::declare(ext);
            flame::time::declare(ext);
            flame::mysql::declare(ext);
            flame::redis::declare(ext);
            flame::mongodb::declare(ext);
            flame::kafka::declare(ext);
            flame::rabbitmq::declare(ext);
            flame::tcp::declare(ext);
            // flame::udp::declare(ext);
            // flame::http::declare(ext);
            // flame::log::declare(ext);
        }
        else
        {
            ext.function<flame::fake_go>("flame\\go",
            {
                {"coroutine", php::TYPE::CALLABLE},
            });
        }
        return ext;
    }
};
#include "../controller.h"
#include "../coroutine.h"
#include "value_body.h"
#include "client_request.h"
#include "client_response.h"
#include "_connection_pool.h"
#include "../url.h"

namespace flame::http
{
    _connection_pool::_connection_pool(uint32_t cph)
    : max_(cph), tm_(gcontroller->context_x), resolver_(gcontroller->context_x) {
    }

    _connection_pool::~_connection_pool() {
    }
    std::shared_ptr<tcp::socket>
    _connection_pool::acquire(std::shared_ptr<url> url, time_t_ ts,coroutine_handler &ch) {
        boost::system::error_code ec_;

        // 超时预处理
        auto cc = false;
        auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(ts - std::chrono::steady_clock::now()).count();
        boost::asio::steady_timer tmo(gcontroller->context_x);
        tmo.expires_after(std::chrono::milliseconds(timeout));
        tmo.async_wait([&cc, &ch, &ec_](const boost::system::error_code &ec) {
            if(ec == boost::asio::error::operation_aborted)
                return;
            cc = true;
            ec_ = ec;
            ch.resume();
        });

        std::shared_ptr<tcp::socket> c;
        auto type = url->schema + url->host + std::to_string(url->port);
        while(p_.count(type) > 0) {
            auto it = p_.find(type);
            if(it != p_.end()) {
                auto s = it->second.first;
                p_.erase(it);
                s->async_wait(tcp::socket::wait_write, ch[ec_]);
                if(!ec_) {
                    c = s;
                    goto DONE;
                }
                // 若连接存在异常，直接释放
                --ps_[type];
            }
        }

        // 未获取到连接，且连接数达到最大
        if(!c) {
            if(ps_[type] >= max_) {
                auto cb = wl_[type].emplace_back([&c, &ch](std::shared_ptr<tcp::socket> s) {
                    c = s;
                    ch.resume();
                });
                ch.suspend();
                // 此处可能由于释放的连接已关闭
                // 导致此处连接为空，而需要重建连接
                if(cc) {
                    for(auto i = wl_[type].begin(); i != wl_[type].end(); ++i ) {
                        if(&(*i) == &cb) {
                            wl_[type].erase(i);
                            break;
                        }
                    }
                }
            }
        }
DONE:
        if(cc == true) {
            throw php::exception(zend_ce_exception, (boost::format("timeout to acquire new connection, host %1%, connections %2%, connection limit %3%") % (url->host + std::to_string(url->port)) % ps_[type] % max_).str());
        }
        tmo.cancel();
        if(c)
            return c;

        return create(url, ts, ch);
    }

    std::shared_ptr<tcp::socket>
    _connection_pool::create(std::shared_ptr<url> u, time_t_ ts, coroutine_handler& ch) {
        boost::system::error_code ec_;
        // 超时预处理
        bool cc = false;
        auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(ts - std::chrono::steady_clock::now()).count();
        boost::asio::steady_timer tmo(gcontroller->context_x);
        tmo.expires_after(std::chrono::milliseconds(timeout));
        tmo.async_wait([&cc, &ch, &ec_] (const boost::system::error_code &ec) {
            if(ec == boost::asio::error::operation_aborted) {
                return;
            }
            cc = true;
            ec_ = ec;
            ch.resume();
        });

        tcp::resolver::results_type eps_;
        auto c = std::make_shared<tcp::socket>(gcontroller->context_x);
        auto type = u->schema + u->host + std::to_string(u->port);
        ++ps_[type];
        resolver_.async_resolve(
            u->host, 
            std::to_string(u->port), 
            [&eps_, &ec_, &cc, &ch] (const boost::system::error_code& ec, tcp::resolver::results_type eps) {
                if(cc){
                    return ;
                }
                ec_ = ec;
                eps_ = std::move(eps);
                ch.resume();
        });
        ch.suspend();
        if(ec_ || cc)
            goto END;

        boost::asio::async_connect(*c, eps_, [&cc, &ch, &ec_] (const boost::system::error_code& ec, const tcp::endpoint& ep) {
            if(cc)
                return ;
            ec_ = ec;
            ch.resume();
        });
        ch.suspend();
END:
        if(cc == true) {
            --ps_[type];
            throw php::exception(zend_ce_exception, (boost::format("timeout to create new connection, host %1%, connections %2%, connection limit %3%") % (u->host + std::to_string(u->port)) % ps_[type] % max_).str());
        } 
        tmo.cancel();
        if(ec_) {
            --ps_[type];
            throw php::exception(zend_ce_exception, (boost::format("failed to create new connection: (%1%) %2%") % ec_.value() % ec_.message()).str(), ec_.value());
        }
        return c;
    }

    php::value _connection_pool::execute(client_request* req, time_t_ ts, coroutine_handler &ch) {
        auto c = acquire(req->url_, ts, ch);

        // 构建并发送请求
        bool cc      = false;
        auto type    = req->url_->schema + req->url_->host + std::to_string(req->url_->port);
        auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(ts - std::chrono::steady_clock::now()).count();
        boost::system::error_code ec_;
        boost::asio::steady_timer tmo(gcontroller->context_x);
        tmo.expires_after(std::chrono::milliseconds(timeout));
        tmo.async_wait([&ch, &cc, &ec_](const boost::system::error_code &ec) {
            if(ec == boost::asio::error::operation_aborted)
                return;
            cc = true;
            ec_ = ec;
            ch.resume();
        });

        boost::beast::http::async_write(*c, req->ctr_, [&cc, &ch, &ec_](const boost::system::error_code& ec, std::size_t n) {
            if(cc == true) return ;
            ec_ = ec;
            ch.resume();
        });
        ch.suspend();
        if(cc) {
            --ps_[type];
            throw php::exception(zend_ce_exception, "failed to write request: timeout");
        }
        if(ec_) {
            --ps_[type];
            throw php::exception(zend_ce_exception, (boost::format("failed to write request: (%1%) %2%") % ec_.value() % ec_.message()).str(), ec_.value());
        }

        // 构建并读取响应
        auto res_ref = php::object(php::class_entry<client_response>::entry());
        auto res_    = static_cast<client_response*>(php::native(res_ref));
        boost::beast::flat_buffer b;
        boost::beast::http::async_read(*c, b, res_->ctr_, [&ch, &cc, &ec_] (const boost::system::error_code& ec, std::size_t n) {
            if(cc == true) return ;
            ec_ = ec;
            ch.resume();
        });

        ch.suspend();
        if(cc) {
            throw php::exception(zend_ce_exception, "failed to read response: timeout");
            --ps_[type];
        }
        if(ec_) {
            throw php::exception(zend_ce_exception, (boost::format("failed to read response: (%1%) %2%") % ec_.value() % ec_.message()).str(), ec_.value());
            --ps_[type];
        }

        res_->build_ex();
        tmo.cancel();

        if(res_->ctr_.keep_alive() && req->ctr_.keep_alive())
            release(type, c);
        else
            release(type, nullptr);
        return res_ref;
    }

    void _connection_pool::release(std::string type, std::shared_ptr<tcp::socket> s) {
        if(!s)
            --ps_[type];
        if (wl_[type].empty()) {
            // 无等待分配的请求
            if(s)
                p_.insert(std::make_pair(type, std::make_pair(s, std::chrono::steady_clock::now())));
        } else {
            // 立刻分配使用
            std::function<void(std::shared_ptr<tcp::socket>)> cb = wl_[type].front();
            wl_[type].pop_front();
            cb(s);
        }
    }
    void _connection_pool::sweep() {
        tm_.expires_from_now(std::chrono::seconds(30));
        tm_.async_wait([this] (const boost::system::error_code &ec) {
            auto now = std::chrono::steady_clock::now();
            if(ec) return; // 当前对象销毁时会发生对应的 abort 错误

            for (auto i = p_.begin(); i != p_.end();) {
                auto itvl = std::chrono::duration_cast<std::chrono::seconds>(now - i->second.second);
                if(itvl.count() > 30) {
                    --ps_[i->first];
                    i = p_.erase(i);
                } else
                    ++i;
            }
            for(auto i = wl_.begin(); i != wl_.end(); ) {
                if(i->second.empty())
                    i = wl_.erase(i);
                else
                    ++i;
            }
            // 再次启动
            sweep();
        });
    }
}


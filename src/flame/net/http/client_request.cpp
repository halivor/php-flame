#include "../../coroutine.h"
#include "client_request.h"
#include "client.h"

namespace flame {
namespace net {
namespace http {
client_request::client_request()
: curl_(nullptr)
, curl_header(nullptr)
, curl_fd(-1)
, cli_(nullptr) {
	poll_ = (uv_poll_t*)malloc(sizeof(uv_poll_t));
	poll_->data = this;
	curl_ = curl_easy_init();
	curl_easy_setopt(curl_, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(curl_, CURLOPT_PRIVATE, this);
}
php::value client_request::__construct(php::parameters& params) {
	if (params.length() >= 3) {
		prop("timeout") = static_cast<int>(params[2]);
	}else{
		prop("timeout") = 2500;
	}
	if (params.length() >= 2) {
		prop("body") = static_cast<php::string&>(params[1]);
		prop("method") = php::string("POST");
	}else{
		prop("method") = php::string("GET");
	}
	if (params.length() >= 1) {
		prop("url") = static_cast<php::string&>(params[0]);
	}
	prop("header") = php::array(0);
	return this;
}

size_t client_request::read_cb(void *ptr, size_t size, size_t nmemb, void *stream) {
	memcpy(ptr, stream, size*nmemb);
	return size*nmemb;
}

size_t client_request::write_cb(char* ptr, size_t size, size_t nmemb, void *userdata) {
	client_request* req = reinterpret_cast<client_request*>(userdata);
	size_t          len = size*nmemb > CURL_MAX_WRITE_SIZE ? CURL_MAX_WRITE_SIZE : size*nmemb;
	std::memcpy(req->buffer_.put(len), ptr, len);
	return len;
}

bool client_request::done_cb(CURLMsg* message) {
	switch(message->msg) {
	case CURLMSG_DONE:
		if (message->data.result != CURLE_OK) {
			co_->fail(curl_easy_strerror(message->data.result), message->data.result);
		} else {
			co_->next(std::move(buffer_));
		}
		return true;
	default:
		assert(0);
		return false;
	}
}

void client_request::build(client* cli) {
	curl_slist* header = build_header();
	if(!header) {
		curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, header);
	}
	this->cli_ = cli;
	// 设置URL
	php::string& url = prop("url");
	if (!url.is_empty()) {
		curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
	} else {
		throw php::exception("client_request build failed: empty url", -1);
	}
	std::string method = prop("method");
	php::value   vbody = prop("body");
	php::string  xbody;
	if (vbody.is_array()) {
		xbody = php::build_query(vbody);
		prop("body") = xbody;
	}else if (vbody.is_string()) {
		xbody = vbody;
	}else if (!vbody.is_null()) {
		xbody = vbody.to_string();
	}
	if(method.compare("POST") == 0) {
		curl_easy_setopt(curl_, CURLOPT_POST, 1L);
		if(xbody.length() > 0) {
			curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, xbody.c_str());
		}
	}else if(method.compare("PUT") == 0) {
		curl_easy_setopt(curl_, CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(curl_, CURLOPT_PUT, 1L);
		if(xbody.length() > 0) {
			curl_easy_setopt(curl_, CURLOPT_READDATA, xbody.c_str());
			curl_easy_setopt(curl_, CURLOPT_INFILESIZE, xbody.length());
			curl_easy_setopt(curl_, CURLOPT_READFUNCTION, read_cb);
		}
	}
	curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, static_cast<long>(prop("timeout")));
	curl_easy_setopt(curl_, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_cb);
	curl_easy_setopt(curl_, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2);
	if (cli->debug_) {
		curl_easy_setopt(curl_, CURLOPT_VERBOSE, 1L);
	}
}

void client_request::close() {
	if (curl_header) {
		curl_slist_free_all(curl_header);
		curl_header = nullptr;
		curl_easy_cleanup(curl_);
		curl_   = nullptr;
		uv_close((uv_handle_t*)poll_, coroutine::free_close_cb);
		poll_ = nullptr;
		curl_fd = -1;
		cli_    = nullptr;
		buffer_.reset();
	}
}
curl_slist* client_request::build_header() {
	if (curl_header) return curl_header;

	php::array header = prop("header");
	if (header.length()) {
		return nullptr;
	} else {
		php::string str;
		for(auto i=header.begin();i!=header.end();++i) {
			php::string& key = i->first;
			php::string& val = i->second;
			php::string  str(key.length() + val.length() + 3);
			sprintf(str.data(), "%.*s: %.*s", key.length(), key.data(), val.length(), val.data());
			curl_header = curl_slist_append(curl_header, str.c_str());
		}
		return curl_header;
	}
}

}
}
}

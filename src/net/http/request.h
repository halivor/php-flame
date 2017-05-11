#pragma once

namespace net {
	class tcp_socket;
	namespace http {
		class request_header_parser;
		class request: public php::class_base {
		public:
			request():header_(std::size_t(0)) {}
			static void init(php::extension_entry& extension);
			static php::value parse(php::parameters& params);
			php::value body(php::parameters& params);
		private:
			void read_head(php::object& req, php::callable& done);
			void read_body(php::object& req, php::callable& done);
			void parse_body();
			bool is_keep_alive();
			php::array             header_;
			boost::asio::streambuf buffer_;
			tcp::socket*           socket_;

			friend class response;
			friend class request_header_parser;
		};
	} }

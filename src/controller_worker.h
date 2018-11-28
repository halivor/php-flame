#pragma once
#include "vendor.h"

namespace flame
{
    class controller_worker {
	public:
		controller_worker();
        void initialize(const php::array& options);
		void run();

	private:
		std::vector<std::thread> thread_;
	};
}
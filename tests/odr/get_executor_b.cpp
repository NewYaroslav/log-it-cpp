#include <logit_cpp/logit.hpp>
#include <memory>

extern "C" logit::detail::TaskExecutor* get_executor_b(){ 
	return std::addressof(logit::detail::TaskExecutor::get_instance());
}
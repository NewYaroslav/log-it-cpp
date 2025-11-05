#include <logit_cpp/logit.hpp>
#include <memory>

extern "C" logit::detail::TaskExecutor* get_executor_a(){ 
	return std::addressof(logit::detail::TaskExecutor::get_instance());
}
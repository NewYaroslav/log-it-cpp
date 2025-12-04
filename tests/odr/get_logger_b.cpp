#include <logit_cpp/logit.hpp>
#include <memory>

extern "C" logit::Logger* get_logger_b(){ 
	return std::addressof(logit::Logger::get_instance());
}
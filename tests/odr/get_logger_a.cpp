#include <logit_cpp/logger.hpp>
#include <memory>

extern "C" logit::Logger* get_logger_a(){ 
	return std::addressof(logit::Logger::get_instance());
}
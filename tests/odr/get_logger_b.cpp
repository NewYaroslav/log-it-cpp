#include <logit_cpp/logger.hpp>
#include <memory>

extern "C" logit::Logger* get_logger_b(){ 
	return std::addressof(logit::Logger::get_instance());
}
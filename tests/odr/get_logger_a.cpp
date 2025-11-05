#include <logit_cpp/logit/Logger.hpp>
#include <memory>

extern "C" logit::Logger* get_logger_a(){ 
	return std::addressof(logit::Logger::get_instance());
}
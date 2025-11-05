#include <iostream>
#include <logit_cpp/logger.hpp>

extern "C" logit::Logger* get_logger_a();
extern "C" logit::Logger* get_logger_b();

int main() {
	auto* logger_a = get_logger_a();
	auto* logger_b = get_logger_b();
	
	std::cout 	<< "Logger A address: " << static_cast<const void*>(logger_a) 
				<< "	Logger B address: " << static_cast<const void*>(logger_b) 
				<< std::endl;
				
	if (logger_a != logger_b) {
		std::cout << "There are 2 different Logger instances!" << std::endl;
		return 1;
	}
	
	std::cout << "There's only Logger, singlton works correctly" << std::endl;
	return 0;
}

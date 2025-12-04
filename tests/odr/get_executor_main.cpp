#include <iostream>
#include <logit_cpp/logit.hpp>

extern "C" logit::detail::TaskExecutor* get_executor_a();
extern "C" logit::detail::TaskExecutor* get_executor_b();

int main() {
	auto* executor_a = get_executor_a();
	auto* executor_b = get_executor_b();
	
	std::cout 	<< " Task Executor address: " << static_cast<const void*>(executor_a) 
				<< "	Task Executor B address: " << static_cast<const void*>(executor_b) 
				<< std::endl;
				
	if (executor_a != executor_b) {
		std::cout << "There are 2 different Task Executor instances!" << std::endl;
		return 1;
	}
	
	std::cout << "There's only Task Executor, singlton works correctly" << std::endl;
	return 0;
}

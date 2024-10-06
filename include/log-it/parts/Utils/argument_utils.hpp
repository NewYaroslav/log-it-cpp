#pragma once
#ifndef _LOGIT_ARGUMENT_UTILS_HPP_INCLUDED
#define _LOGIT_ARGUMENT_UTILS_HPP_INCLUDED
/// \file argument_utils.hpp
/// \brief Functions for working with arguments and converting them to value arrays.

#include "VariableValue.hpp"
#include <tuple>
#include <vector>

namespace logit {

	/// \brief Base case of recursion for argument conversion — when there are no more arguments.
	/// \param name_iter Iterator for the argument name list.
	/// \return An empty vector, as there are no more arguments to process.
	inline std::vector<VariableValue> args_to_array(std::vector<std::string>::const_iterator name_iter) {
		return {};
	}

	/// \brief Recursive function to convert arguments into an array of tuples (name, value).
	/// \tparam T Type of the first argument.
	/// \tparam Ts Types of the remaining arguments.
	/// \param name_iter Iterator for the argument name list.
	/// \param first_arg The first argument.
	/// \param args The remaining arguments.
	/// \return A vector of tuples containing argument names and values.
	template <typename T, typename... Ts>
	std::vector<VariableValue> args_to_array(std::vector<std::string>::const_iterator name_iter, const T& first_arg, const Ts&... args) {
		// Создаем вектор и добавляем первый элемент (имя, значение)
		std::vector<VariableValue> result;
		result.push_back(VariableValue(*name_iter, first_arg));
		name_iter++;
		// Рекурсивно добавляем оставшиеся элементы
		auto tail_result = args_to_array(name_iter, args...);
		result.insert(result.end(), tail_result.begin(), tail_result.end());
		return result;
	}

	using crev_it_t = std::string::const_reverse_iterator;

	/// \brief Checks if the '>' character is the closing of a template argument list.
	/// \param left_it Iterator pointing to the '>' character.
	/// \param right_it Iterator pointing to the end of the string.
	/// \return true if the '>' character closes a template argument list, false otherwise.
	bool is_closing_template(crev_it_t left_it, crev_it_t right_it) {
		if (*left_it != '>' || left_it == right_it) return false;
		--left_it; // move to right
		while (left_it != right_it && (
				*left_it == ' '
				|| *left_it == '\t'
				|| *left_it == '\n'
				|| *left_it == '\r'
				|| *left_it == '\f'
				|| *left_it == '\v')) {
			--left_it;
		}
		return *left_it == ':' && (left_it != right_it) && *(left_it-1) == ':';
	}

	/// \brief Splits a string of argument names into individual names, ignoring nested templates, parentheses, and quotes.
	/// \param all_names The string containing all argument names.
	/// \return A vector of strings with individual argument names.
	inline std::vector<std::string> split_arguments(const std::string& all_names) {
		auto result = std::vector<std::string>{};
		if (all_names.empty()) return result;

		auto right_cut = all_names.crbegin();
		auto left_cut = all_names.crbegin();
		auto const left_end = all_names.crend();
		auto parenthesis_count = int{0};
		auto angle_bracket_count = int{0};

		// Parse the arguments string backward. It is easier this way to check if a '<' or
		// '>' character is either a comparison operator, or a opening and closing of
		// templates arguments.
		for (;;) {
			if (left_cut != left_end && (left_cut+1) != left_end) {
				// Ignore commas inside quotations (single and double)
				if (*left_cut == '"' && *(left_cut+1) != '\\') {
					// Don't split anything inside a string
					++left_cut;
					while (!( // Will iterate with left_cut until the stop condition:
							(left_cut+1) == left_end						// The next position is the end iterator
							|| (*left_cut == '"' && *(left_cut+1) != '\\')	// The current char the closing quotation
						)) {
						++left_cut;
					}
					++left_cut;
				} else
				if (*left_cut == '\'' && *(left_cut+1) != '\\') {
					// Don't split a ',' (a comma between single quotation marks)
					++left_cut;
					while (!(  // Will iterate with left_cut until the stop condition:
							(left_cut+1) == left_end						 // The next position is the end iterator
							|| (*left_cut == '\'' && *(left_cut+1) != '\\')	 // The current char is the closing quotation
						)) {
						++left_cut;
					}
					++left_cut;
				}
			}

			if (left_cut == left_end ||
				(*left_cut == ',' && parenthesis_count == 0 && angle_bracket_count == 0)){
				// If it have found the comma separating two arguments, or the left ending
				// of the leftmost argument.

				// Remove the leading spaces
				auto e_it = left_cut - 1;
				while (*e_it == ' ') --e_it;
				++e_it;

				// Remove the trailing spaces
				while (*right_cut == ' ') ++right_cut;

				result.emplace(result.begin(), e_it.base(), right_cut.base());
				if (left_cut != left_end) {
					right_cut = left_cut + 1;
				}
			} else
			// It won't cut on a comma within parentheses, such as when the argument is a
			// function call, as in IC(foo(1, 2))
			if (*left_cut == ')') {
				++parenthesis_count;
			} else
			if (*left_cut == '(') {
				--parenthesis_count;
			} else
			// It won't cut on a comma within a template argument list, such as in
			// IC(std::is_same<int, int>::value)
			if (is_closing_template(left_cut, right_cut)) {
				++angle_bracket_count;
			} else if (*left_cut == '<' && angle_bracket_count > 0) {
				--angle_bracket_count;
			}

			if (left_cut == left_end) {
				break;
			} else {
				++left_cut;
			}
		}
		return result;
	}

}; // namespace logit

#endif // _LOGIT_ARGUMENT_UTILS_HPP_INCLUDED

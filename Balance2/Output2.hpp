//
//  Output2.hpp
//  Balance
//
//  Created by William Hurwood on 5/8/25.
//
#ifndef Output_hpp
#define Output_hpp

#include <cstdio>
#include <filesystem>
#include <format>

/**
 Minor class used to collect and format output data.
 C++20, C++23 note: We will use this class to experiment with new options for formatting output.
 */
class Output2
{
public:
	Output2();
	Output2(const std::filesystem::path& path);
	
	// Accessors
	void set_happy_path(bool value) { _happy_path = value; }
	bool happy_path() const { return _happy_path; }
	void set_unique_happy_path(bool value) { _unique_happy_path = value; }
	bool unique_happy_path() const { return _unique_happy_path; }

	// Change the indentation level
	void indent();
	void outdent();
	
	// Write methods
	Output2& operator<<(const std::string line);
	template <class... Args>
	Output2& println(std::format_string<Args...> fmt, Args&&... args)
	{
		// Support formatted output of a line
		return operator<<(std::format(fmt, std::forward<Args>(args)...));
	}

private:
	FILE* destination;						// Location into which we write the output (file or stdout)
	std::string prefix;						// Prefix written to indent output
	bool _happy_path = false;			// If true only display weighings contributing to the result
	bool _unique_happy_path = true;			// If true only show one happy path at each level
};

#endif /* Output_hpp */

//
//  Output.cpp
//  Balance
//
//  Created by William Hurwood on 5/8/25.
//
#include <print>
#include <cassert>
#include "Output2.hpp"

// Default constructor uses standard output
Output2::Output2()
	: destination(stdout)
{}

// Alternatively caller can supply a file name
// C++17 note: We now have a class used to represent a path
Output2::Output2(const std::filesystem::path& path)
	: destination(fopen(path.c_str(), "w"))
{}

void Output2::indent()
{
	prefix.push_back(' ');
	prefix.push_back(' ');
}

void Output2::outdent()
{
	assert(prefix.size() >= 2);
	prefix.pop_back();
	prefix.pop_back();
}

Output2& Output2::operator<<(const std::string line)
{
	// Output a single line putting the prefix string in front of it
	std::println(destination, "{}{}", prefix, line);
	return *this;
}

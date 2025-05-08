//
//  Output.cpp
//  Balance
//
//  Created by William Hurwood on 5/8/25.
//
#include <print>
#include <cassert>
#include "Output.hpp"

// Default constructor uses standard output
Output::Output()
	: destination(stdout)
{}

// Alternatively caller can supply a file name
// C++17 note: We now have a class used to represent a path
Output::Output(const std::filesystem::path& path)
	: destination(fopen(path.c_str(), "w"))
{}

void Output::indent()
{
	prefix.push_back(' ');
	prefix.push_back(' ');
}

void Output::outdent()
{
	assert(prefix.size() >= 2);
	prefix.pop_back();
	prefix.pop_back();
}

Output& Output::operator<<(const std::string line)
{
	// Output a single line putting the prefix string in front of it
	std::println(destination, "{}{}", prefix, line);
	return *this;
}

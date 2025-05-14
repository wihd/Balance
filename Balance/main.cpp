//
//  main.cpp
//  Balance
//
//  Created by William Hurwood on 4/25/25.
//
#include <print>
#include "Manager.hpp"
#include "ProblemFindMajority.hpp"
#include "Output.hpp"

constexpr uint8_t coint_count = 3;

int main(int argc, const char * argv[]) {
	// Solve a problem
	ProblemFindMajority problem(coint_count);
	Manager<ProblemFindMajority> manager(problem, coint_count);
	std::println("Finished execution with {} nodes", manager.solve_breadth(9));

	// Output our state
	Output output("/Users/will/Desktop/output.log");
	output.set_only_happy_path(true);
	manager.write(output);
	return 0;
}

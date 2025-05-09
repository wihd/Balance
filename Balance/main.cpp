//
//  main.cpp
//  Balance
//
//  Created by William Hurwood on 4/25/25.
//
#include "Manager.hpp"
#include "ProblemFindMajority.hpp"
#include "Output.hpp"

constexpr uint8_t coint_count = 5;

int main(int argc, const char * argv[]) {
	// Solve a problem
	ProblemFindMajority problem(coint_count);
	Manager<ProblemFindMajority> manager(problem, coint_count);
	manager.solve_breadth(4);
	
	// Output our state
	Output output{};
	manager.write(output);
	return 0;
}

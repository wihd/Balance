//
//  main.cpp
//  Balance
//
//  Created by William Hurwood on 4/25/25.
//
#include "Manager.hpp"
#include "ProblemFindMajority.hpp"
#include "Output.hpp"

int main(int argc, const char * argv[]) {
	// Solve a problem
	ProblemFindMajority problem(3);
	Manager<ProblemFindMajority> manager(problem, 3);
	manager.solve_breadth(3);
	
	// Output our state
	Output output{};
	manager.write(output);
	return 0;
}

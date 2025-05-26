//
//  main.cpp
//  Balance2
//
//  Created by William Hurwood on 5/14/25.
//

// Balance is unable to handle problem instances of size 11 and above
// Since the approach is exponential we will never be able to handle larger problem instances
// But we think we can support 11 by agressively pruning our node tree
// We observe that many nodes contain equivalent states so we want to
// 1. Observe that solving an equivalent state is sufficient to solve our state
// 2. Only handle one state from each equivalency class

// Maybe this will make c=11 feasible.  My goal is to cover c=15, but I must say that this
// seems to be unlikely to be achievable.
#include <print>
#include "Manager2.hpp"
#include "ProblemFindMajority2.hpp"
#include "Output2.hpp"

constexpr uint8_t coint_count = 7;

int main(int argc, const char * argv[]) {
	// Instantiate a manager to solve a specific problem
	Manager2<ProblemFindMajority2> manager(coint_count);
	std::println("Coin count: {}", coint_count);
	std::println("Graph size at each depth: {}", manager.solve_breadth(coint_count));

	// Output our state
	Output2 output;
	output.set_happy_path(true);
	manager.write(output);
	return 0;
}

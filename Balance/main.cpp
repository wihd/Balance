//
//  main.cpp
//  Balance
//
//  Created by William Hurwood on 4/25/25.
//

#include <iostream>
#include "Manager.hpp"
#include "ProblemFindMajority.hpp"

int main(int argc, const char * argv[]) {
	// insert code here...
	std::cout << "Hello, World!\n";
	
	ProblemFindMajority problem(9);
	Manager<ProblemFindMajority> manager(problem);
	return 0;
}

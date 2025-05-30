Project contains C++ code to investigate solving puzzles using a balance to identify coins.  e are given a set of coins that can only be distinguished by weight, along with an objective and constraints on coins that might be available.  We attempt to find optimal ways to meet the ojective.

At this time we have two subprojects `Balance` and `Balance2`.  Both are trying to solve the problem by brute force, but the search space is just too large to explore this way.  For `Balance2` we are much more agressive at identifying similar states.  This considerably reduces the size of the search space (for the problem with nine coins it reduces the number of states to explore by a factor of over 70).  But the issue remains that we still cannot obtain an answer to `n=11` size problem.

Next we will give up on attempting to explore all coins and instead to search for good solutions.

//
//  StateTemplates.h
//  Balance
//
//  Created by William Hurwood on 5/15/25.
//

#ifndef StateTemplates_h
#define StateTemplates_h

#include <concepts>
#include "Types2.h"

// We declare templates and concepts that will be used both by Problem classes and by the Manager
// that tracks solving a problem
class Partition2;

/**
 ProblemState records information known about a problem after viewing some sequence of weighings and their outcomes.
 Here we declare a concept to encapsulate what must be true for a ProblemState.
 
 A ProblemState always has a partiton, and some other information.  The nature of the other information
 will depend on the problem we want to solve.
 */
template<class T>
concept ProblemState = std::totally_ordered<T> && requires(T state)
{
	// The state must have a datamember called `partition` which is a raw pointer to a Partition2 object
	// The partition groups together coins that either cannot be distinguished (because every weighing that
	// led to the state placed the coins in the same pan) or which the problem does not want to distinguish
	// (because the problem has deduced that these coins are identical).
	// When considering weighings that we might apply to the state the manager only needs to consider the
	// number of coins selected from each part.
	// The partition belongs to the cache of partitions owned by Partition2 class, so the ProblemState
	// does not need to consider its memory management.  We may also assume that two partitions are
	// equal if and only if they are equal as pointers, since all cached partitions are different.
	// I'm not entirely clear what is happening here.  If T is a class with a data member `partition` declared
	// with type `Partition2*` then compiler seems to determine type of { state.partiton } as `Partition2*&`.
	// Using same_as<Partition2*> fails compilation
	// Using same_as<Partition2*&> passes compilation, but seems wrong to me
	// So I'm writing std::convertible_to<Partition2*> which passes compilation presumably because a value
	// of type `Partition2*&` can be converted to `Partition2*`.  But where did & come from?
	// See https://stackoverflow.com/questions/74474371/concept-that-requires-a-certain-return-type-of-member
	// for an explanation of what is happening here.  I will keep convertible_to<> but the answer to that
	// question tells us how we could constrain the class to have `Partition2*`.  What it comes down to
	// is that the compiler is constraining the declaraed type of the given expression, which is actually
	// a reference to the expression.
	{ state.partition } -> std::convertible_to<Partition2*>;
};

/**
 Problem encapsulates the logic used to represent a specific problem.
 It is a class which (presumably) stores settings about the problem.
 It contains a number of methods that the Manager will invoke to:
 - obtain default state of the problem
 - compute state when applying a weighing
 - provide information about the nature of a problem state
 - write output information about the problem
 */
template <class P>
concept Problem = requires(P problem,
						   P::StateType state_value,
						   Partition2* partition)
{
	// We expect that P will define a type of a state of this problem
	typename P::StateType;
	
	// The StateType type of a problem must meet requirements of ProblemState
	{ state_value } -> ProblemState;
	
	// The problem must have a method make_root() that returns a suitable state for the root of the tree
	// We expect the Manager to supply us with the root partition
	{ problem.make_root(partition) } -> std::same_as<typename P::StateType>;
};

#endif /* StateTemplates_h */

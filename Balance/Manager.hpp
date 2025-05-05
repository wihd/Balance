//
//  Manager.hpp
//  Balance
//
//  Created by William Hurwood on 4/30/25.
//

#ifndef Manager_hpp
#define Manager_hpp

#include <concepts>
#include <memory>
#include "Types.h"
#include "PartitionCache.hpp"

/// Sentinel depth value to represent a node at which we have not yet determined the depth
constexpr uint8_t NOT_RESOLVED = 255;

// C++20 Note: We will use a concept to define a suitable Problem class
// It would have been simpler to just use an interface, but I want to have a go at concepts.
template<typename T>
concept Problem = requires(
    T problem,									// problem is a parameter with type of class we are testing
    const Partition& partition,					// parameter that specifies a partition
    const Weighing& weighing,					// parameter that specifies a weighing
    const PartitionProvenance& provanence,		// parameter that specifies a provanence
    const typename T::StateType& state_value,	// parameter specifies value returned by apply_weighing
    uint8_t coin_count)							// coint_count paramter
{
	// Specify that T has a typename parameter called StateType
	// The problem will use this to track what it knows after some sequence of weighings and outcomes
	// TODO: Do we need to specify additional constaints for this type?  E.g. is it assignable? default constructable?
	// And if we do, we need to determine the correct syntax for it
	typename T::StateType;

	// Problem must have a function to return the datatype value at the root of the tree
	// Specify that T has a member function called make_root_data(...)
	// - Its possible the root value depends on the number of coins, so we pass it into function
	// - Result of the function must be the same type as DataType
	{ problem.make_root_data(coin_count) } -> std::same_as<typename T::StateType>;

	// Problem tracks what happens when we perform a weighing
	// - Result is fixed length array giving data for each possible result of the weighing
	{ problem.apply_weighing
		(
		 partition,								// partition to which weighing is applied
		 weighing,								// the weighing we apply
		 partition,								// partition generated as result of weighing
		 provanence								// the provenance of parts in output partition
		)
	} -> std::same_as<OutcomeArray<typename T::StateType>>;
	
	// Problem can examine the data (for some partition) and determine if problem is resolved
	// There are two ways a problem might be resulted at some partition
	// - Ideally the problem is solved here
	// - Possibly this state is impossible
	// The manager does not care which of these - either way it need not explore further
	{ problem.is_resolved(partition, state_value) } -> std::same_as<bool>;
};

// Structure used to store information about a single node the manager is tracking
// The node describes the outcome of a single weighing and stores nodes for weighings based on this outcome
// We expect that S is the StateType of some Problem.
template <typename S>
struct Node
{
	// The node represents the application of some implied weighing to an implied partition
	// This induces a new implied partition.  All of these objects are stored in the partition cache.
	// There are three possible outcomes.  For each outcome the problem will want to store some state.
	OutcomeArray<S> state;
	
	// The child nodes of this node, if any
	// If this data member is not nullptr then it points to an array of nodes
	// Each array has length given by number of weighings for this node's partition
	// We have separate arrays for each outcome since it is possible that some outcomes are directly resolved
	// but others need to be expanded.  So we do not want to use same array of nodes for each child
	OutcomeArray<std::unique_ptr<Node<S>[]>> children;

	// The problem is resolved (for an outcome) if either
	// 1. The Problem object reports that the state is resolved (we store 0), or
	// 2. There is a child weighing for which the problem is resolved (we store child+1)
	// We store a sentinel value if the problem is not resolved
	OutcomeArray<uint8_t> resolved_depth = { NOT_RESOLVED, NOT_RESOLVED, NOT_RESOLVED };
};

// Base class for the manager
// This class holds parts of the manager that are independent of the problem
// TODO: This class may be pointless
class ManagerBase
{
public:
private:
	PartitionCache cache;
};

/**
 The manager class handles the state of the program during a test execution.
 It is configured with a Strategy object.
 It asks the strategy object to judge whether or not it is making progress towards a solution to whatever
 problem is being solved.
 
 The goal is a decision tree.  Each node of the tree is represented by a specific weighing.
 Each weighing has three outcome branches which lead to either
 - An outcome (i.e. problem is solved), or
 - The next weighing
 
 Our goal is to find a decision tree with minimal depth.  The manager tracks parts of a tree.
 It uses the strategy object to:
 - Determine the outcome after a weighing (i.e. problem solved, or undecided)
 - If the streategy can manage it, a measure of how much the problem is partially solved
 
 The Manager knows the depth of each node, and how many lines have already been closed off.
 */
template <Problem P>
class Manager : public ManagerBase
{
public:
	Manager(P& p) : problem(p) {}
	void solve(uint8_t coin_count);
	
private:
	P& problem;
	Node<typename P::StateType> root;
};

#endif /* Manager_hpp */

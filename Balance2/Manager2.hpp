//
//  Manager2.hpp
//  Balance
//
//  Created by William Hurwood on 5/17/25.
//

#ifndef Manager2_h
#define Manager2_h

#include <memory>
#include <vector>
#include <map>

// The Manager template class is instantiated with a problem class instance
// It will track problem states and ask the Problem class instance to compute the outcome of weighings
// against these states.  The problem is solved if it can identify a sequence of weighings that
// will solve every possible outcome.  The branching factor is exponential.  To attempt to claw
// back one or two extra coins before it becomes unfeasible we expect the problem to return not the
// state that actually resolves but a representative equivalent state.  We expect many different
// paths to lead to the same state.  We are hoping for a significant reduction in the number of
// states that we need to consider.
#include "StateTemplates.h"
#include "Types2.h"

// Special resolved depth value that means that (so far) we have no estimate of resolved depth of node
constexpr uint8_t DEPTH_INFINITY = 255;



/**
 Manager class searches the state space of a problem looking for the shortest path to solve the problem.
 */
template <Problem P>
class Manager2
{
	// The key type records potential information that we might have deduced about the problem
	using Key = std::unique_ptr<typename P::StateType>;
	
	// Structure that records information about a known, interesting, child state of a node
	struct Child
	{
		// For each of the three outcomes, record the resulting state
		// We will use nullptr if an outcome leads to an impossible state
		// The state object will be owned as a key of the manager's states map.
		OutcomeArray<typename P::StateType*> state;
		
		// For reporting we would like to know what weighing should be done (using list of weighings for
		// the parent partition) to get to these states.  At solve time however this information is not needed.
		// We could reduce memory footprint by excluding this number and then finding it again on
		// the solution tree.
		int weighing_number;
	};

	// The manager uses an instance of this structure to track the processing, if any, it has done
	// to a problem state.
	struct Status
	{
		// Vector listing the interesting child states of this node
		// The potential children are obtained by examining this node's partition which gives a list of weighings.
		// Our current approach is that we eliminate as many potential children as possible on expansion.
		// Either attempt to solve problem used fixed length array here, but we feel that in fact most
		// children of a node are equivalent so we should switch to a sparse list of children.
		std::vector<Child> children;
		
		// Record depth below this node at which the problem can be solved.
		// So value is 0 if this node records a state at which the problem is solved.
		// Value is $n$ if this node has a child for which all of the outcomes are either impossible
		// or have a resolved depth that is at most $n-1$.
		// We use special value DEPTH_INFINITY if we have not found a suitable child.
		// Note that a problem is always solvable from any state given sufficiently many weighings.
		uint8_t resolved_depth = DEPTH_INFINITY;
	};
	
	using StatesType = std::map<Key, Status, PointerComparator<typename P::StateType>>;
	
	// Iterator class used to record a position in the tree of nodes
	class Iterator
	{
	public:
		Iterator(Manager2<P>& manager) : states(manager.states), path{manager.root} {}

	private:
		std::vector<typename StatesType::iterator> path;
		std::vector<size_t> child_numbers;
		StatesType& states;
	};
	friend class Iterator;

public:
	// Forward all of the parameters from the constructor to the unknown problem object
	template <typename... Args>
	Manager2(Args&&... args) : problem(std::forward<Args>(args)...), root(states.end()) {}
	void clear();
	int solve_breadth(uint8_t stop_depth);

private:
	P problem;						// This class contains the problem specific logic
	StatesType states;
	StatesType::iterator root;
	
	// Helper methods
	size_t expand(const Iterator& node);
};

template<Problem P>
void Manager2<P>::clear()
{
	// Erase the cache, and replace it with one containing just the root state
	states.clear();
	states.emplace(problem.make_root(), Status{});
	
	// Since the root state is only state in the map it must be its begin() state
	// Of course as soon as we insert any more states the position of the root is non-deterministic
	root = states.begin();
}

template<Problem P>
int Manager2<P>::solve_breadth(uint8_t stop_depth)
{
	// Start by clearing the graph, but expand the root node
	// When we return the root node we will know that we have completed the iteration
	clear();
	expand(Iterator(*this));
	return 0;
}

template <Problem P>
size_t Manager2<P>::expand(const Iterator& node)
{
	// Ensure that the node has full compliment of children, computing its depth if possible
	// Return the number of children added to the node (if it was already expanded we return 0)
	return 0;
}

#endif /* Manager2_h */

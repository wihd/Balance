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
#include <cassert>

// The Manager template class is instantiated with a problem class instance
// It will track problem states and ask the Problem class instance to compute the outcome of weighings
// against these states.  The problem is solved if it can identify a sequence of weighings that
// will solve every possible outcome.  The branching factor is exponential.  To attempt to claw
// back one or two extra coins before it becomes unfeasible we expect the problem to return not the
// state that actually resolves but a representative equivalent state.  We expect many different
// paths to lead to the same state.  We are hoping for a significant reduction in the number of
// states that we need to consider.
#include "StateTemplates.h"
#include "Partition2.hpp"
#include "Types2.h"

// Special resolved depth value that means that (so far) we have no estimate of resolved depth of node
constexpr uint8_t DEPTH_INFINITY = 255;


// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Declare class Manager2

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
		OutcomeArray<typename P::StateType*> keys;
		
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
		
		// A solution for a node consists of a tree of children of the node such that each node's
		// immediate children cover all of the outcomes of a single weighing, and every leaf is solved.
		// The depth of the solution is the length of the longest path from the node to a leaf.
		// The resolved depth of the node is the minimum possible depth of all solutions to the node.

		// We record an upper bound for this node's resolved depth
		// The special value DEPTH_INFINITY means that we do not have any ceiling on the resolved depth
		// Setting this data member to 0 would indicate that the node is already solved
		// As we explore the tree we may be able to reduce the `resolved_maximum`.
		// If we find a weighing such that all three outcomes are either omitted or have a maximum depth
		// of at most $d$ then we can infer that this node's maximum depth is at most $d+1$.
		uint8_t depth_max = DEPTH_INFINITY;
		
		// We record a lower bound for this node's resolved depth
		// Raising the lower bound is hard because we must consider all of the node's children.
		// To set out minimum depth to $d+1$ we must show that every child weighing has at least one
		// outcome whose minimum depth is at least $d$.
		uint8_t depth_min = 0;
		
		// We always have depth_min <= depth_max.  If we have equality then we have resolved the depth
		// for this node.  If we have resolved the depth for the root then we have solved the problem!
		bool is_resolved() const { return depth_min == depth_max; }
		
		// On expanding a node either we will give the node children, or we will set depth to 0
		// Only way a node can have a depth that isn't infinity is if it has been expanded - very minor optimisation!
		bool is_expanded() const { return depth_max != DEPTH_INFINITY || !children.empty(); }
	};
	
	using StatesType = std::map<Key, Status, PointerComparator<typename P::StateType>>;
	
	// Iterator class used to record a position in the tree of nodes
	class Iterator
	{
	public:
		// Constructor starts with iterator at root of tree
		Iterator(Manager2<P>& manager) : states(manager.states), path{manager.root} {}

		// Accessors
		bool is_root() const { return path.size() == 1; }
		size_t depth() const { return child_numbers.size(); }
		typename P::StateType& key() const { return *path.back()->first.get(); }
		Status& value() const { return path.back()->second; }

		// Modifiers
		bool advance_first_child();
		bool advance_parent();
		bool advance_sibling();
		void advance_prune();

	private:
		std::vector<typename StatesType::iterator> path;
		std::vector<size_t> child_numbers;
		std::vector<Outcome> outcomes;
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


// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// class Manager2<P>::Iterator

template <Problem P>
bool Manager2<P>::Iterator::advance_first_child()
{
	// Attempt to modify iterator by moving to its first expanded child
	// Return true if made change
	// return false if no children (and guarantee that iterator is not changed)
	auto& children = value().children;
	
	// If the node has no children (either because we did not expand or it was already solved) then we cannot advance
	if (children.empty())
	{
		return false;
	}
	
	// Otherwise find the first outcome of this child that has a non-null key
	// We never create a child unless it has at least two keys, so we will always find one
	auto& child = children.front();
	for (int outcome = Outcome::Begin; outcome != Outcome::End; ++outcome)
	{
		if (child.keys[outcome])
		{
			// Advance the iterator to this child
			path.push_back(states.find(child.keys[outcome]));
			assert(path.back() != states.end());
			child_numbers.push_back(0);
			outcomes.push_back(static_cast<Outcome>(outcome));
			return true;
		}
	}
	
	// If we get here something is wrong!
	assert(false);
	return false;
}

template <Problem P>
bool Manager2<P>::Iterator::advance_parent()
{
	// Attempt to modify iterator by moving to its parent
	// Return true if made change, or false if there was no parent (i.e. started at root)
	// Promise iterator is not changed if it returns false
	if (is_root())
	{
		return false;
	}
	else
	{
		path.pop_back();
		child_numbers.pop_back();
		outcomes.pop_back();
		return true;
	}
}

template <Problem P>
bool Manager2<P>::Iterator::advance_sibling()
{
	// Attempt to modify iterator by moving to a sibling node - that is the next existing node after the current
	// node within its parent.  This method does NOT attempt to move to a cousin node
	// Nodes that were never constructed are presumed to not exist - the advance methods will not move to them.
	// Return true if made change
	// return false if the node has no further sibling (iterator is not changed)
	
	// The root node has no siblings
	if (is_root())
	{
		return false;
	}
	
	// If we are not the root we must be able to examine the children of our parent node
	auto& children = path[path.size() - 2]->second.children;
	auto child_number = child_numbers.back();
	auto& child = children[child_number];
	
	// The weighing used to get this node might have another outcome
	for (int outcome = static_cast<int>(outcomes.back()) + 1; outcome != Outcome::End; ++outcome)
	{
		if (child.keys[outcome])
		{
			// We have another child of the same weighing
			path.back() = states.find(child.keys[outcome]);
			assert(path.back() != states.end());
			// child_numbers was not changed
			outcomes.back() = static_cast<Outcome>(outcome);
			return true;
		}
	}

	// If our parent node has no more weighings then it cannot have any more children
	if (++child_number == children.size())
	{
		return false;
	}

	// There is another wighing, since weighings are always non-void, it has another child
	auto& next_child = children[child_number];
	for (int outcome = Outcome::Begin; outcome != Outcome::End; ++outcome)
	{
		if (next_child.keys[outcome])
		{
			// We have another child from the next weighing
			path.back() = states.find(next_child.keys[outcome]);
			assert(path.back() != states.end());
			child_numbers.back() = child_number;
			outcomes.back() = static_cast<Outcome>(outcome);
			return true;
		}
	}
	
	// If we get here something is wrong!
	assert(false);
	return false;
}

template <Problem P>
inline void Manager2<P>::Iterator::advance_prune()
{
	// We attempt to advance the iterator as little as possible, without ever reaching a child
	// to obtain a new node.  This means it will call advance_parent() zero or more times, followed
	// by a sucessful call to advance_sibling().
	// This method will place the iterator on the root if it cannot find another node
	while (!advance_sibling() && advance_parent())
	{}
}


// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// class Manager2<P>

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
	auto& key = node.key();
	auto& status = node.value();
	
	// Since the "tree" is really a DAG we will often visit nodes multiple times
	// An attempt to expand a node that was already expanded does nothing
	if (status.is_expanded())
	{
		return 0;
	}
	
	// The purpose of expanding a node is to potentially tighten the resolved depth of this node
	// or one of its ancestors.  If that cannot happen then we should not expand this node.
	// TODO: Work out implications of this
	// We are concerned that if we visit every node with every possible path of ancestors we could
	// easily do exponential work because there can be $O(2^x)$ paths through a DAG with $O(x)$ nodes.
	// So we should visit each node only a fixed number of times, pruning whenever we see a node
	// again during the same pass.
	// In that case examining our path of ancestors must be incorrect, since it would just be which ever
	// path happened to occur first in visitation order.  So we will omit such a step.
	
	// Each child of the node will correspond to a weighing
	// We cache the distinct weighings for each partition (omitting some weighings which are always
	// solvable by symmetry of the balance).  Iterate through each of these weighings.
	Partition2* partition = key.partition;
	for (auto [weighing, output_partition] : partition->get_children())
	{
		// Ask the problem to determine the output state (for each outcome) when applying this weighing to our state
		auto outcomes = problem.apply_weighing(key, weighing, output_partition);
	}
	return 0;
}

#endif /* Manager2_h */

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
#include <algorithm>

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
#include "Weighing2.hpp"
#include "Types2.h"
#include "Output2.hpp"

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
		OutcomeArray<typename P::StateType*> keys{nullptr, nullptr, nullptr};
		
		// For reporting we would like to know what weighing should be done (using list of weighings for
		// the parent partition) to get to these states.  At solve time however this information is not needed.
		int weighing_number = -1;
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
		
		// Output the depth numbers as a single line on output
		std::string format_resolution() const;
		void write_resolution(Output2& output) const { output.println("Status:    {}", format_resolution()); }
	};
	
	using StatesType = std::map<Key, Status, PointerComparator<typename P::StateType>>;
	using StatesIterator = StatesType::iterator;
	
	// Iterator class used to record a position in the tree of nodes
	class Iterator
	{
	public:
		// Constructor starts with iterator at root of tree
		Iterator(Manager2<P>& manager) : states(manager.states), path{manager.root} {}
		
		// Tell compiler on assignment we will just assume that the states parameter is untouched
		Iterator& operator=(const Iterator& other)
		{
			path = other.path;
			child_numbers = other.child_numbers;
			outcomes = other.outcomes;
			assert(&states == &other.states);
			return *this;
		}

		// Accessors
		bool is_root() const { return path.size() == 1; }
		size_t depth() const { return child_numbers.size(); }
		typename P::StateType& key() const { return *path.back()->first.get(); }
		typename P::StateType* key_ptr() const { return path.back()->first.get(); }
		Status& value() const { return path.back()->second; }
		size_t child_number() const { return child_numbers.back(); }
		decltype(auto) parents_children() const { return path[path.size() - 2]->second.children; }
		int weighing_number() const { return parents_children()[child_number()].weighing_number; }
		Outcome outcome() const { return outcomes.back(); }
		Partition2* input_partition() const { return path[path.size() - 2]->first.get()->partition; }
		decltype(auto) weighing() const { return input_partition()->get_children()[weighing_number()]; }
		Partition2* output_partition() const { return weighing().output; }

		// Modifiers
		bool advance_first_child();
		bool advance_parent();
		bool advance_outcome();
		bool advance_sibling();
		void advance_prune();

	private:
		std::vector<StatesIterator> path;		// Iterators to all nodes on the path, always includes root
		std::vector<size_t> child_numbers;		// For non-root nodes; index number in parent of this child
		std::vector<Outcome> outcomes;			// For non-root nodes; outcome taken from parent's child
		StatesType& states;						// Reference to states belonging to our Manager
	};
	friend class Iterator;
	
	using NodeIdMap = std::map<typename P::StateType*, int>;

public:
	// Forward all of the parameters from the constructor to the unknown problem object
	template <typename... Args>
	Manager2(Args&&... args) : problem(std::forward<Args>(args)...), root(states.end()) {}
	void clear();
	std::vector<size_t> solve_breadth(uint8_t stop_depth);
	void write(Output2& output);

private:
	P problem;						// This class contains the problem specific logic
	StatesType states;
	StatesIterator root;
	NodeIdMap node_ids;				// On writing, store map assigning ids to nodes
	
	// Helper methods
	void expand(const Iterator& node);
	void improve_node(Iterator& node, uint8_t target_depth);
	bool write_weighing(Output2& output, Iterator& node);
	void write_node(Output2& output, Iterator& node);
};


// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// class Manager2<P>::Status

template <Problem P>
std::string Manager2<P>::Status::format_resolution() const
{
	if (depth_max == 0)
	{
		return "Leaf node";
	}
	else if (is_resolved())
	{
		return std::format("Optimal solution at depth {}", depth_max);
	}
	else if (depth_max < DEPTH_INFINITY)
	{
		return std::format("Solution at depth {}; explored to depth {}", depth_max, depth_min);
	}
	else
	{
		return std::format("No solution; explored to depth {}", depth_min);
	}
}


// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// class Manager2<P>::Iterator

template <Problem P>
bool Manager2<P>::Iterator::advance_first_child()
{
	// Attempt to modify iterator by moving to its first child
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
bool Manager2<P>::Iterator::advance_outcome()
{
	// Attempt to modify iterator by moving to a sibling node which is a different outcome of the SAME weighing
	// node within its parent.  This method does NOT attempt to move to a cousin node or to another weighing
	// Return true if made change
	// return false if the node has no further outcome (iterator is not changed)
	
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
			// We have another outcome child of the same weighing
			path.back() = states.find(child.keys[outcome]);
			assert(path.back() != states.end());
			// child_numbers was not changed
			outcomes.back() = static_cast<Outcome>(outcome);
			return true;
		}
	}

	// There were no more outcomes, so we cannot make the desired advance
	return false;
}

template <Problem P>
bool Manager2<P>::Iterator::advance_sibling()
{
	// Attempt to modify iterator by moving to a sibling node - that is the next existing node after the current
	// node within its parent.  This method does NOT attempt to move to a cousin node
	// Return true if made change
	// return false if the node has no further sibling (iterator is not changed)
	
	// If we can advance to another outcome of our current weighing then we are done
	if (advance_outcome())
	{
		return true;
	}
	
	// The root node has no siblings
	if (is_root())
	{
		return false;
	}
	
	// If we are not the root we must be able to examine the children of our parent node
	auto& children = path[path.size() - 2]->second.children;
	auto child_number = child_numbers.back();
	
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
std::vector<size_t> Manager2<P>::solve_breadth(uint8_t stop_depth)
{
	// Start by clearing the graph, but expand the root node
	// When we return the root node we will know that we have completed the iteration
	clear();
	Iterator root_iterator(*this);
	expand(root_iterator);
	std::vector<size_t> graph_sizes{1, states.size()};

	// We perform iterations from the root, each of which must increase its depth_min by one
	// We stop when we have found a minimal solution to the problem
	auto& root_status = root_iterator.value();
	while (!root_status.is_resolved() && root_status.depth_min < stop_depth)
	{
		improve_node(root_iterator, root_status.depth_min + 1);
		graph_sizes.push_back(states.size());
	}
	return graph_sizes;
}

template <Problem P>
void Manager2<P>::improve_node(Iterator& node, uint8_t target_depth)
{
	// Ensure that the input node is either resolved, or its depth_min is not smaller than the target
	// We may expand the node and visit its children
	// The iterator may change within this method, but it should always have same value on exit as it did no entry
	
	// Ensure that input node is expanded (method does nothing if node is solved, or if it was already expanded)
	expand(node);
	
	// We will exit immediately if the goal is already met
	auto& status = node.value();
	if (status.is_resolved() || status.depth_min >= target_depth)
	{
		return;
	}

	// The current status of our child nodes does not permit us to meet the goal for this node
	// We must have child nodes (since a solved node always meets the goal)
	// So we will improve each child node
	assert(!status.children.empty());
	assert(target_depth > 0);
	node.advance_first_child();
	uint8_t worst_depth_min = DEPTH_INFINITY;
	do {
		// Group together the children that come from the same weighing
		uint8_t worst_depth_max = 0;
		do {
			// Improve the child node, but use a smaller target depth
			// This process must stop because expanding a node will either resolve it or set its depth_min to 2 or better
			// We are using recursion as against iteration.  The depth of the recursion is at most the number of weighings
			// in the solution, which will be a small integer so we should not risk stack overflow.
			improve_node(node, target_depth - 1);

			// The depth_max of the weighing is the worst depth_max of its three outcomes
			worst_depth_max = std::max(worst_depth_max, node.value().depth_max);
			
			// Unless the child is resolved it might also worsen (i.e. reduce) the min depth
			if (!node.value().is_resolved())
			{
				worst_depth_min = std::min(worst_depth_min, node.value().depth_min);
			}
		} while (node.advance_outcome());

		// Our depth_min can only be improved when we have considered every weighing
		// But our depth_max can be improved as we go along
		if (worst_depth_max != DEPTH_INFINITY && worst_depth_max + 1 < status.depth_max)
		{
			status.depth_max = worst_depth_max + 1;
			assert(status.depth_min <= status.depth_max);

			// Since we just improved our depth_max it is possible that our node is now resolved
			if (status.is_resolved())
			{
				// We have made the required improvement, so we can exit now without looking at other children
				// TODO: Ideally problem would let us sort children to increase likelyhood of exiting here
				node.advance_parent();
				return;
			}
		}
	} while (node.advance_sibling());
	node.advance_parent();
	
	// We have considered every child, so now we can increase our depth_min
	// I think it is extremely implausible that we have a new value that is better than our target
	if (worst_depth_min == DEPTH_INFINITY)
	{
		// Every single child was resolved, so this node should be resolved as well
		assert(status.depth_max != DEPTH_INFINITY);
		status.depth_min = status.depth_max;
	}
	else
	{
		// We encountered at least one child that was not resolved, so we have a new estimate for depth_min
		// It is possible that some other child reduced the depth_max to sufficiently that we are now resolved
		status.depth_min = std::min(static_cast<uint8_t>(worst_depth_min + 1), status.depth_max);
	}
	assert(status.depth_min <= status.depth_max);
	assert(status.depth_min >= target_depth);
}

template <Problem P>
void Manager2<P>::expand(const Iterator& node)
{
	// Ensure that the node has full compliment of children, computing its depth if possible
	// Return the number of children added to the node (if it was already expanded we return 0)
	auto& key = node.key();
	auto& status = node.value();
	
	// Since the "tree" is really a DAG we will often visit nodes multiple times
	// An attempt to expand a node that was already expanded does nothing
	if (status.is_expanded())
	{
		return;
	}

	// We do not attempt to consider whether or not to expand this node based on path taken to reach it
	// This makes no sense.  There could be an exponential number of paths to the same node (and in fact
	// cycles are not impossible).  So instead we will expand the node regardless of path taken.
	
	// If one weighing leads to same outcomes as another then there is no point in tracking both of them
	// We will store locally a set of the outcomes we have seen to detect duplicates.
	// We are not sure how many duplicates will occur
	std::set<OutcomeArray<typename P::StateType*>> seen_combinations;
	
	// We want to compute an improved min-depth for this node
	// The minimum depth for a node is one more than the minimum value of all of the min depths of children
	// except that if a child's min-depth == max-depth then we treat its minimum depth as infinity.
	uint8_t worst_child_min_depth = DEPTH_INFINITY;
	
	// Each child of the node will correspond to a weighing
	// We cache the distinct weighings for each partition (omitting some weighings which are always
	// solvable by symmetry of the balance).  Iterate through each of these weighings.
	Partition2* partition = key.partition;
	int weighing_number = -1;
	for (auto [weighing, output_partition] : partition->get_children())
	{
		// Ask the problem to determine the output state (for each outcome) when applying this weighing to our state
		++weighing_number;
		auto outcomes = problem.apply_weighing(key, weighing, output_partition);

		// It is permitted to omit a weighing if we can conclude that following the weighing will not help.
		// We expect the problem to omit an outcome if it is 'impossible' meaning that based on the input
		// state it can conclude that the outcome never occurs.
		int impossible_count = 0;
		for (auto& outcome : outcomes)
		{
			if (!outcome)
			{
				++impossible_count;
			}
		}

		// If the weighing has two impossible outcomes then it follows that the weighing provides no more
		// information.  In that case we will omit it from the list of weighings to explore.
		// Note that it is logically inconsistent for all three outcomes to be impossible
		// But we permit this case to allow for a slight optimization.  If the problem finds two impossible
		// outcomes then its pointless for it to compute the third since we already know manager will ignore it.
		// A weighing with one impossible outcome is perfectly valid.
		if (impossible_count >= 2)
		{
			continue;
		}
		
		// If the weighing is symmetric then then the left and right outcomes will always be equally
		// hard to solve (by symmetry).  So in that case we will discard one of them now.
		if (weighing->is_symmetric(*output_partition))
		{
			// TODO: It is unfortunate that we are making manager allocate object here that it knew we would delete
			// Note that a symmetric weighing with left & right outcomes permitted, but balanced impossible
			// is perfectly useful.  We omitted right outcome because it is redundant to explore it
			// not because it is impossible.
			outcomes[Outcome::RightHeavier].reset();
		}
		
		// We will now look-up standard instances for all remaining outcomes so pointer compare will work
		// This will never result in unused entries in states since we only omit outcomes if they are duplicates
		OutcomeArray<typename P::StateType*> child_keys{nullptr, nullptr, nullptr};
		uint8_t deepest_outcome = 0;
		for (int i = Outcome::Begin; i != Outcome::End; ++i)
		{
			if (outcomes[i])
			{
				// Get a node for this outcome as a key, emplacing a new one if there is no existing one
				// C++17 Note: This method will not allocate new pair if map already contains an object
				auto insert_result = states.try_emplace(std::move(outcomes[i]), Status{});
				auto& child_status = insert_result.first->second;
				if (insert_result.second)
				{
					// We have never seen this state before - determine if the problem is solved for it
					if (problem.is_solved(*insert_result.first->first.get()))
					{
						// Record that we have a solved state
						// If we need to agressively save memory we observer we don't need multiple solved states
						child_status.depth_max = 0;
						
						// No change to worst_child_min_depth since this node is solved
					}
					else
					{
						// We have no information about depth of this outcome except it is not 0
						// Since worst_child_min_depth can never be zero it must go to 1
						deepest_outcome = DEPTH_INFINITY;
						worst_child_min_depth = child_status.depth_min = 1;
					}
				}
				else
				{
					// Since this is an existing state we already have some information on its resolved depth
					deepest_outcome = std::max(deepest_outcome, child_status.depth_max);
					if (child_status.depth_min < child_status.depth_max)
					{
						worst_child_min_depth = std::min(worst_child_min_depth, child_status.depth_min);
					}
				}

				// Record the child node (as a raw pointer key value - its owned by `states`)
				child_keys[i] = insert_result.first->first.get();
			}
		}
		
		// If all three outcomes of this weighing are solved (or were omitted entirely because impossible/duplicate)
		// then this weighing leads to a solution in one step regardless of its outcome.
		// Since this node is not solved (otherwise we would have said it was already expanded) we have found
		// the best possible course of action from this node.
		if (deepest_outcome == 0)
		{
			// We now know this node's resolved depth
			status.depth_min = status.depth_max = 1;
			
			// There is no point in continuing to expand this node's children since we cannot find a better weighing
			// Similarly there is no point in recording any of the other weighings we examined
			status.children = std::vector<Child>{Child{child_keys, weighing_number}};
			return;
		}

		// It is possible that several outcomes lead to equivalent states
		// When that happens we have a minor optimization to only track one of them
		// We took this optimization away since it has no effect on memory footprint and
		// the effect on time is negligable since on visiting second node we always find it to be the
		// already resolved.  But keeping it makes output easier to understand.
//		if (child_keys[0] == child_keys[1])
//		{
//			child_keys[1] = nullptr;
//			if (child_keys[0] == child_keys[2])
//			{
//				child_keys[2] = nullptr;
//			}
//		}
//		else if (child_keys[0] == child_keys[2])
//		{
//			child_keys[2] = nullptr;
//		}
//		else if (child_keys[1] == child_keys[2])
//		{
//			child_keys[2] = nullptr;
//		}

		// It is possible that multiple weighings lead to same set of outcome states in which case we skip them
		// Note that having one repeated outcome is not sufficient - we need to be able to say that the
		// weighings are equivalent, not that the weighings have an equivalent outcome
		OutcomeArray<typename P::StateType*> probe = child_keys;
		std::sort(probe.begin(), probe.end());
		if (seen_combinations.insert(probe).second)
		{
			// This is a fresh combination so we will create a child for it
			status.children.emplace_back(child_keys, weighing_number);
			
			// Even if the new child was not solved, it is possible (but unlikely) that it does let us improve the depth
			// NB: We already checked deepest_outcome != 0, so it is safe to subtract one from it without underflow
			if (deepest_outcome < DEPTH_INFINITY && status.depth_max > deepest_outcome + 1)
			{
				status.depth_max = deepest_outcome + 1;
			}
		}
	}
	
	// Our resolved depth must be at least 2 because if it were 0 then this node would be solved, and if it were 1
	// then this node would have had a child that was solved for all outcomes and we did not find such a child.
	// Its just possible (if all our child states were either already partially resolved or were fresh solved states)
	// that we can record that resolved depth is bigger than 2 but its unlikely.
	if (worst_child_min_depth == DEPTH_INFINITY)
	{
		// This means that for every child was fully resolved - it had a finite max_depth and we knew that
		// it was not possible to do better.  In that case this node is also now fully resolved.
		// We set the depth_max for this node as we went along, so we can set min_depth for it as well
		assert(status.depth_max != DEPTH_INFINITY);
		status.depth_min = status.depth_max;
	}
	else
	{
		// We found at least one child whose resolved depth is unknown.
		// It remains possible that exploring that child would improve on our current max_depth
		// All we can say now is that our depth lower bound is at least one larger than that of the child
		// If we actually found a (necessarily different) child that gave us a shorter path to a solution
		// then our min depth is that value
		status.depth_min = std::min(static_cast<uint8_t>(worst_child_min_depth + 1), status.depth_max);
	}
}

template <Problem P>
void Manager2<P>::write(Output2& output)
{
	output << "Manager: {";
	output.indent();
	problem.write_description(output);
	root->second.write_resolution(output);
	
	// Describe the state of the root node
	Iterator node(*this);
	node.key().partition->write(output);
	problem.write_ambiguous_state(output, node.key());
	
	// Our 'tree' is really a graph but we do not want to list each node more than once
	// So we will assign numbers to nodes as we visit them
	// If we visit a node already in map we will just output its number and prune it
	node_ids = NodeIdMap{{root->first.get(), 0}};

	// Iterate over all our children
	if (node.advance_first_child())
	{
		output << "Children:  [";
		output.indent();
		do {
			if (write_weighing(output, node))
			{
				// When writing happy path we do not print more than one weighing child of a node
				break;
			}
		} while (node.advance_sibling());

		// Close the Children scope
		output.outdent();
		output << "]";
	}
	else
	{
		output << "Children:  <Root has not been expanded>";
	}
	
	// End the Manager object
	output.outdent();
	output << "}";
}

template <Problem P>
bool Manager2<P>::write_weighing(Output2& output, Iterator& node)
{
	// On entry the iterator will point to a non-root node (it should be first node with current weighing)
	// We output a description of the node's weighing, and then visit each recorded outcome of the node
	// On exit the iterator will point to a node reachable from entry node via advance_outcome()
	// and which cannot advance its outcome any more
	// Return true if we want to prune our siblings (because we wrote on happy path)
	
	// Did caller only ask to show happy path
	if (output.happy_path())
	{
		// We will only show this node if all three of its outcomes are resolved
		// Of course its possible that this node is resolved, but it is not the best output
		// But we believe that this approach will filter out most false paths
		Iterator save_node = node;
		bool all_resolved = node.value().is_resolved();
		while (node.advance_outcome())
		{
			all_resolved &= node.value().is_resolved();
		}
		if (all_resolved)
		{
			// Move back to saved position to allow method to show content
			node = save_node;
		}
		else
		{
			// Prune this weighing (note we have already advanced to last outcome as required)
			return false;
		}
	}
	
	auto& weighing = node.weighing();
	output.println("{{ // child_index={:<6} weighing_number={}", node.child_number(), node.weighing_number());
	output.indent();
	weighing.weighing->write(output, *weighing.output);
	weighing.output->write(output);

	// Iterate over each of the childen
	do {
		write_node(output, node);
	} while (node.advance_outcome());
	
	// End the weighing grouping
	output.outdent();
	output << "}";
	return output.happy_path() && output.unique_happy_path();
}

template <Problem P>
void Manager2<P>::write_node(Output2& output, Iterator& node)
{
	// Output a description of the current node, starting with its outcome within its weighing
	
	// If we have visited the node before we just output one line and do not attempt to write any more
	if (node_ids.find(node.key_ptr()) != node_ids.end())
	{
		output.println("{} Revisited #{:<6} depth={:<3} status=<{}>",
					   outcome_names[node.outcome()],
					   node_ids[node.key_ptr()],
					   node.depth(),
					   node.value().format_resolution());
		return;
	}
	
	// Otherwise this is first visit to this node
	int node_id = static_cast<int>(node_ids.size());
	node_ids[node.key_ptr()] = node_id;
	output.println("{} Node #{:<8} depth={:<4} {{",
				   outcome_names[node.outcome()],
				   node_id,
				   node.depth());
	output.indent();
	node.value().write_resolution(output);
	
	// Write a summary of the state of the node
	
	// We already wrote (as part of the weighing) the partition induced by the weighing
	// But the node might have merged some parts - if it did we write the node's partition here
	if (node.output_partition() != node.key().partition)
	{
		node.key().partition->write(output);
	}
	
	// If the node is solved here we call a different output function
	if (node.value().depth_max == 0)
	{
		problem.write_solved_node(output, node.key());
	}
	else
	{
		problem.write_ambiguous_state(output, node.key());
	}
	
	// If the node has children we write them now
	if (node.advance_first_child())
	{
		output << "Children:  [";
		output.indent();
		do {
			if (write_weighing(output, node))
			{
				break;
			}
		} while (node.advance_sibling());

		// Close the Children scope
		output.outdent();
		output << "]";
		node.advance_parent();
	}
	
	// End the description of this node
	output.outdent();
	output.println("}} // Node: {}", node_id);
}

#endif /* Manager2_h */

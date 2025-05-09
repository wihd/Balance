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
#include <algorithm>
#include <cassert>
#include "Types.h"
#include "PartitionCache.hpp"
#include "Output.hpp"

/// Sentinel depth value to represent a node at which we have not yet determined the depth
constexpr uint8_t NOT_RESOLVED = 255;

/// Format a resolved depth with special strings for most interesting values
inline std::string format_resolved_depth(int depth)
{
	switch (depth)
	{
		case 0:
			return "<Resolved Here>";
		case NOT_RESOLVED:
			return "<Not Resolved>";
		default:
			return std::format("<Longest path: {}>", depth);
	}
}

// C++20 Note: We will use a concept to define a suitable Problem class
// It would have been simpler to just use an interface, but I want to have a go at concepts.
template<typename T>
concept Problem = requires(
    T problem,										// problem is a parameter with type of class we are testing
    const Partition& partition,						// parameter that specifies a partition
    const Weighing& weighing,						// parameter that specifies a weighing
    const PartitionProvenance& provanence,			// parameter that specifies a provanence
	Output& output,									// Object to which we can write output state
	const char* name,								// For output, sometimes pass in a name string
	const typename T::StateType state_value,		// parameter for the state type without a reference
    const typename T::StateType& state_reference)	// parameter specifies value returned by apply_weighing
{
	// Specify that T has a typename parameter called StateType
	// The problem will use this to track what it knows after some sequence of weighings and outcomes
	typename T::StateType;
	
	// We initialise arrays of the state type and then move values into it
	// These requirements failed to compile - not clear why, so erase them for now
	//	{ state_value } -> std::default_initializable;
	//	{ state_value } -> std::movable;

	// Problem must have a function to return the datatype value at the root of the tree
	// Specify that T has a member function called make_root_data(...)
	// - Result of the function must be the same type as DataType
	{ problem.make_root_data() } -> std::same_as<typename T::StateType>;

	// Problem tracks what happens when we perform a weighing
	// - Result is fixed length array giving data for each possible result of the weighing
	{ problem.apply_weighing
		(
		 partition,								// partition to which weighing is applied
		 state_reference,						// The state before this weighing
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
	{ problem.is_resolved(partition, state_reference) } -> std::same_as<bool>;
	
	// Sometimes we might want to distinguish between impossible and solved
	{ problem.is_impossible(partition, state_reference) } -> std::same_as<bool>;
	
	// We provide multiple options to add a desription of the problem or of a state of the problem
	{ problem.write_description(output) };
	{ problem.write_solved_node(output, partition, state_reference, name) };
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
	
	// The node as a whole is resolved at the depth of its deepest outcome
	uint8_t resolved_depth_all() const
	{
		// C++17 note: Have function to find largest member of any container
		return *std::max_element(resolved_depth.begin(), resolved_depth.end());
	}
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
class Manager
{
	using NodeType = Node<typename P::StateType>;
	
	// Iterator used to navigate the tree of Nodes
	class NodeIterator
	{
	public:
		// The iterator starts at the root of the tree
		NodeIterator(PartitionCache& cache, uint8_t coin_count, Node<typename P::StateType>& root) :
			cache(cache),
			nodes{&root},
			partitions{cache.get_root(coin_count)}
		{}
		
		bool is_root() const { return nodes.size() == 1; }
		size_t depth() const { return indexes.size(); }
		NodeType& node() const { return *nodes.back(); }
		std::pair<NodeType*, Outcome> ancestor(size_t height) const
		{
			// height = 0 returns immediate ancestor, so valid in range [0, depth).
			return {nodes[nodes.size() - height - 2], outcomes[outcomes.size() - height - 1]};
		}
		const Partition& partition() const { return *partitions.back(); }
		const Partition& partition(int index) const
		{
			return index >= 0 ? *partitions[index] : *partitions[partitions.size() + index];
		}
		int index() const { return indexes.back(); }
		
		// Modifiers
		bool advance_first_child();
		bool advance_parent();
		bool advance_sibling();
		void advance_prune();
		
	private:
		// We need the cache to interpret the nodes
		PartitionCache& cache;

		// The iterator points to back() node of the vector
		// The back() partition is the partition of this node
		// The other vectors (indexes, outcomes) are one shorter, and describe how we got to this partition
		// Each time we transitioned from an earlier partition we did it by applying some weighing
		// (given as an index number) to one of the outcomes from the earlier partition
		std::vector<NodeType*> nodes;
		std::vector<const Partition*> partitions;
		std::vector<int> indexes;
		std::vector<Outcome> outcomes;
	};

public:
	Manager(P& p, uint8_t coin_count) : problem(p), coin_count(coin_count)
	{
		// We initialize our root node
		// Each node represents three possible states depending on the outcome of the previous weighing.
		// Since the root does not come from a weighing two of its outcomes are not used.
		// We will mark them as already resolved.
		root.resolved_depth[Outcome::LeftHeavier] = 0;
		root.resolved_depth[Outcome::RightHeavier] = 0;
		
		// The state of the third outcome is the one we are using - the problem tells us how to initialise it
		// We may assume that the problem is not resolved - a problem resolved at the root is not interesting!
		root.state[Outcome::Balances] = problem.make_root_data();
	}
	void solve_breadth(uint8_t stop_depth);
	void write(Output& output);
	
private:
	P& problem;
	NodeType root;
	PartitionCache cache;
	uint8_t coin_count;
	
	// Helper methods
	void expand(const NodeIterator& node_it);
	void write_node(Output& output, NodeIterator& node, int& node_counter);
};

template <Problem P>
bool Manager<P>::NodeIterator::advance_first_child()
{
	// Attempt to modify iterator by moving to its first expanded child
	// Return true if made change
	// return false if no children (and guarantee that iterator is not changed)
	auto& node = nodes.back();

	// We consider the nodes children to consist of all of its outcomes, taken together
	// It is possible that first outcome was not expanded but second was expanded (because we do not expand
	// a node if it is already resolved) so we consider each outcome before we give up
	for (int outcome = Outcome::Begin; outcome != Outcome::End; ++outcome)
	{
		if (node->children[outcome])
		{
			// The childrens array is never empty, so if we have an array it has a child
			nodes.push_back(&node->children[outcome][0]);
			partitions.push_back(cache.get_weighings(partitions.back()).partitions[0]);
			indexes.push_back(0);
			outcomes.push_back(static_cast<Outcome>(outcome));
			return true;
		}
	}
	return false;
}

template <Problem P>
bool Manager<P>::NodeIterator::advance_parent()
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
		nodes.pop_back();
		partitions.pop_back();
		indexes.pop_back();
		outcomes.pop_back();
		return true;
	}
}

template <Problem P>
bool Manager<P>::NodeIterator::advance_sibling()
{
	// Attempt to modify iterator by moving to a sibling node - that is the next existing node after the current
	// node within its parent.  This method does NOT attempt to move to a cousin node
	// Return true if made change
	// return false if the node has no further sibling (iterator is not changed)
	
	// The root node has no siblings
	if (is_root())
	{
		return false;
	}
	
	// If we are not the root we must have a parent node to examine
	auto parent_depth = nodes.size() - 2;
	auto& node = nodes[parent_depth];
	int outcome = outcomes.back();

	// Does the current outcome have another child we have not visited yet?
	auto& weighing_items = cache.get_weighings(partitions[parent_depth]);
	int next_index = indexes.back() + 1;
	if (weighing_items.weighings.size() > next_index)
	{
		nodes.back() = &node->children[outcome][next_index];
		partitions.back() = weighing_items.partitions[next_index];
		indexes.back() = next_index;
		// outcomes.back() is unchanged
		return true;
	}

	// There might be another outcome, in which case we take its first child
	for (++outcome; outcome != Outcome::End; ++outcome)
	{
		if (node->children[outcome])
		{
			// If the children array exists then it is non-empty
			nodes.back() = &node->children[outcome][0];
			partitions.back() = weighing_items.partitions[0];
			indexes.back() = 0;
			outcomes.back() = static_cast<Outcome>(outcome);
			return true;
		}
	}
	return false;
}

template <Problem P>
inline void Manager<P>::NodeIterator::advance_prune()
{
	// We attempt to advance the iterator as little as possible, without ever reaching a child
	// to obtain a new node.  This means it will call advance_parent() zero or more times, followed
	// by a sucessful call to advance_sibling().
	// This method will place the iterator on the root if it cannot find another node
	while (!advance_sibling() && advance_parent())
	{}
}

template<Problem P>
void Manager<P>::solve_breadth(uint8_t stop_depth)
{
	// Solve the problem using a depth first search
	// Expand the root node immediately - so we can use the root as a sentinel value
	expand(NodeIterator(cache, coin_count, root));
	
	// Keep incrementing the depth searched, until the root node reports it is resolved
	for (size_t depth = 1;
		 root.resolved_depth[Outcome::Balances] == NOT_RESOLVED && depth != stop_depth;
		 ++depth)
	{
		// We will walk through all of our nodes using an iterator
		NodeIterator iterator(cache, coin_count, root);
		
		// When the iterator returns to the root we have completed a pass through
		iterator.advance_first_child();
		while (!iterator.is_root())
		{
			// Is the iterator at the correct depth?
			if (iterator.depth() == depth)
			{
				expand(iterator);
				
				// Change the iterator, but do not visit any nodes added by this expansion
				iterator.advance_prune();
			}
			else
			{
				// Change the iterator, going deeper if possible
				if (!iterator.advance_first_child())
				{
					iterator.advance_prune();
				}
			}
		}
	}
}

template <Problem P>
void Manager<P>::expand(const NodeIterator& node_it)
{
	// Unless the computation is unhelpful, ensure that the children of this node are computed
	
	// It is possible that this node has an ancestor which is already resolved at such a depth that
	// could not be improved even if this node has a weighing that resolves the problem immediately
	// If that's the case we should not expand this node
	size_t depth = node_it.depth();
	for (size_t h = 0; h != depth; ++h)
	{
		auto [node, outcome] = node_it.ancestor(h);
		if (node->resolved_depth[outcome] <= h + 2)
		{
			// Let d = absolute depth of this node.  We are looking at an ancestor with absolute depth d - h - 1.
			// If ancestor is resolved at r, then the deepest resolved node on best path from the ancestor
			// is at absolute depth d - h - 1 + r.
			// If expanding this node finds an immediate child that is fully resolved then it will be at absolute
			// depth d + 1.  This is the shallowest depth below this node where we might find a resolved node.
			// So for descendents of this node to improve on the resolved depth of the ancestor we need
			// d + 1 < d - h - 1 + r, i.e. we need h + 2 < r
			// So if r <= h + 2 then expanding this node cannot lead to a shallower tree and we abort now
			return;
		}
	}
	
	// Collect some values out of the outcome loop
	auto& node = node_it.node();
	auto& partition = node_it.partition();
	auto& weighing_items = cache.get_weighings(&partition);
	uint8_t original_resolved_depth = node.resolved_depth_all();

	// Note that if all three of the outcomes at this node had resolved depth 0 then our immediate ancestor
	// would already have resolved depth <= 1.  So we would already have exited in that case
	assert(original_resolved_depth > 0);

	// Consider each possible outcome of this node
	for (int outcome = Outcome::Begin; outcome != Outcome::End; ++outcome)
	{
		// If the problem is already resolved for this outcome of the node do not expand it
		// Just in case do not expand again if it is already expanded here
		if (node.resolved_depth[outcome] != NOT_RESOLVED || node.children[outcome])
		{
			continue;
		}
		
		// Allocate an array of nodes into which we will store the result
		// C++14 Note: There is a specialization to allocate an array of node values
		auto children = std::make_unique<NodeType[]>(weighing_items.weighings.size());
		
		// Loop over all of the weighings permitted from this partition
		for (size_t i = 0; i != weighing_items.weighings.size(); ++i)
		{
			// Apply the problem to this weighing
			children[i].state = problem.apply_weighing(partition,
													   node.state[outcome],
													   *weighing_items.weighings[i],
													   *weighing_items.partitions[i],
													   *weighing_items.provenances[i]);
			int resolved_count = 0;
			for (int o = Outcome::Begin; o != Outcome::End; ++o)
			{
				if (problem.is_resolved(partition, children[i].state[o]))
				{
					++resolved_count;
					children[i].resolved_depth[o] = 0;
				}
			}
			
			// If all possible outcomes for this weighing are immediately resolved, then we are now
			// able to resolve our original node at depth 1
			if (resolved_count == Outcome::Count)
			{
				node.resolved_depth[outcome] = 1;
			}
		}
		node.children[outcome] = std::move(children);
	}
	
	// As a result of the expansion it is possible that we might be able to reduce the resolved depth
	// of ancestors of this node.  We will walk up the ancestor tree until we find a node that does not improve
	if (original_resolved_depth > 1 && node.resolved_depth_all() == 1)
	{
		// Observe that since all we changed were nodes whose resolved depth used to be NOT_RESOLVED to 1
		// the only possible change we could have made would be to make the resolved depth == 1 now.
		// This corresponds to resolved depth of 2 for the first parent
		uint8_t new_resolved_depth = 2;
		
		// Walk up the parents of this node
		for (size_t h = 0; h != depth; ++h)
		{
			auto [n, o] = node_it.ancestor(h);
			if (n->resolved_depth[o] <= new_resolved_depth)
			{
				// The ancestor's resolved depth for this outcome is not worse than new setting
				// So there is nothing to change
				break;
			}
			
			// We will make a change here
			auto start_resolved_all = n->resolved_depth_all();
			n->resolved_depth[o] = new_resolved_depth;
			
			// Did this change reduce the combined resolved depth of this node
			auto end_resolved_all = n->resolved_depth_all();
			if (end_resolved_all > start_resolved_all)
			{
				// The expansion from other outcomes at this node means we did not improve outcome further
				break;
			}
			
			// The depth for next ancestor will be one more
			// Note that this number does not need to be (h + 2) (but it cannot be less than h+2)
			// Suppose we started with max{4, 3, 3}=4 and we reduced the 4 to 2.  The new result is max{2, 3, 3} = 3.
			// This is why we need new_resolved_depth instead of just using h+2
			new_resolved_depth = end_resolved_all + 1;
		}
	}
}

template <Problem P>
void Manager<P>::write(Output& output)
{
	output << "Manager: {";
	output.indent();
	problem.write_description(output);
	output.println("Level:     {}", format_resolved_depth(root.resolved_depth[Outcome::Balances]));
	output << "Children:  {";
	output.indent();
	
	// We will number each node as we visit it
	int node_counter = 0;
	NodeIterator node(cache, coin_count, root);
	node.advance_first_child();
	while (!node.is_root())
	{
		write_node(output, node, node_counter);
		
		// Visit nodes in depth first order
		if (node.advance_first_child())
		{
			output.indent();
		}
		else if (!node.advance_sibling())
		{
			while (node.advance_parent())
			{
				output.outdent();
				if (node.advance_sibling())
				{
					break;
				}
			}
		}
	}
	
	// End the Manager object (note depth first outdented one more than it indented)
	output << "}";
	output.outdent();
	output << "}";
}

template <Problem P>
void Manager<P>::write_node(Output& output, NodeIterator& node, int& node_counter)
{
	// Write out information for this node and its descendants
	// On exit we expect both the indentation and node iterator to be at their entry value
	// although both of them may have changed during the function
	assert(!node.is_root());

	// Identify the node
	output.println("Node: {:>5}     depth={:<2} {{", ++node_counter, node.depth());
	output.indent();

	// Specify information that applies to node as a whole - its weighing, its induced partition, its resolved levels
	auto& node_ref = node.node();
	auto& parent_partition = node.partition(-1);
	auto& weighing_items = cache.get_weighings(&parent_partition);
	weighing_items.weighings[node.index()]->write(output, parent_partition);
	node.partition().write(output);
	output.println("Outcomes:  Left: {};  Right: {};  Balances: {}",
				   format_resolved_depth(node_ref.resolved_depth[Outcome::LeftHeavier]),
				   format_resolved_depth(node_ref.resolved_depth[Outcome::RightHeavier]),
				   format_resolved_depth(node_ref.resolved_depth[Outcome::Balances]));

	// We generate information for each of the three outcomes
	for (int outcome = Outcome::Begin; outcome != Outcome::End; ++outcome)
	{
		// If outcome is impossible there is nothing more to be said about it
		if (problem.is_impossible(node.partition(), node_ref.state[outcome]))
		{
			output.println("{} <Cannot occur>", outcome_names[outcome]);
			continue;
		}
		
		// If the node is resolved for this outcome, but not impossible then it must have been solved
		// The problem has a method to display a solved node (which can decide how many lines to use)
		if (node_ref.resolved_depth[outcome] == 0)
		{
			problem.write_solved_node(output, node.partition(), node_ref.state[outcome], outcome_names[outcome]);
			continue;
		}
	}

	// The node is over
	output.outdent();
	output << "}";
}

#endif /* Manager_hpp */

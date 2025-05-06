//
//  Types.h
//  Balance
//
//  Created by William Hurwood on 4/29/25.
//

// We declare some simple types that we want to be common to entire application

#ifndef Types_h
#define Types_h

#include <vector>
#include <array>

/** Enumerate the options for handling a coin in a weighing */
typedef enum : int {
	LeftPan,
	RightPan,
	SetAside,
} Placement;

/**
 A part provance describes the origin of a part in a Partition that was made from a weighing
 */
struct PartProvenance
{
	// We need to be able to compare instances of this structure so we can totally order PartitionProvenance
	auto operator<=>(const PartProvenance&) const = default;
	
	/// The part (of the base partition) used to obtain this part
	int part;
	
	/// The location of this part in the most recent weighing
	Placement placement;
};

/**
 A partition provance lists the provance of each its parts in order
 We guarantee that the members of the provenance vector are always sorted, first by part number
 and then by placement.  Each part will appear at least once.
 */
typedef std::vector<PartProvenance> PartitionProvenance;

/// Array listing the three possible outcomes of a weighing operation
typedef enum : int {
	LeftHeavier,
	RightHeavier,
	Balances,
	
	// Constants that make code more readable
	Begin = 0,
	End = 3,
	Count = 3,
} Outcome;

/// Fixed length array to store the three possible outcomes of a sorting
template <class D>
using OutcomeArray = std::array<D, Outcome::Count>;

#endif /* Types_h */

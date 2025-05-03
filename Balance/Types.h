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
	/// The part (of the base partition) used to obtain this part
	int part;
	
	/// The location of this part in the most recent weighing
	Placement placement;
};

/**
 A partition provance lists the provance of each its parts in order
 */
typedef std::vector<PartProvenance> PartitionProvenance;


#endif /* Types_h */

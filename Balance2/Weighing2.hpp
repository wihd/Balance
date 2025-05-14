//
//  Weighing2.hpp
//  Balance2
//
//  Created by William Hurwood on 5/14/25.
//

#ifndef Weighing2_hpp
#define Weighing2_hpp

#include <vector>
#include "Types2.h"

/**
 Class represents a way of weighing a partition by selecting coins for the left and right pans.
 We observed that the PartitionProvenance class was more useful as a representation of the weighing
 than explicitly listing which coins are in which parts.  So internally this class records the provenance
 represention which means for each output part it records which input part and pan it uses.
 
 To make sense of this class the caller must be aware of both the input and output partitions.
 This class does not specify how many coins are placed into each part.  It is not possible to
 deduce this information given an instance of this class and the input partition.
 
 In first version the output parts were listed in same order as input partition (possible split if the
 part was refined).  In the second version we require the parts of a partition to be in ascending order.
 This means that it is no longer true that output parts split from the same input part are necessarily
 placed together.
 */
class Weighing2
{
public:
	// Specify which coins are placed in the output partition
	struct Part
	{
		auto operator<=>(const Part&) const = default;
		int part;					// Input part number whose coins are sent into this output part
		Placement placement;		// The pan into which these coins were placed during the weighing
	};
	
	auto operator<=>(const Weighing2&) const = default;

	// Accessors
	const Part& provenance(size_t index) const { return provenances[index]; }
	
private:
	std::vector<Part> provenances;	// Array contains an entry for each part in output partition
};

#endif /* Weighing2_hpp */

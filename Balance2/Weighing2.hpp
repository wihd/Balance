//
//  Weighing2.hpp
//  Balance2
//
//  Created by William Hurwood on 5/14/25.
//

#ifndef Weighing2_hpp
#define Weighing2_hpp

#include <vector>
#include <set>
#include "Types2.h"
class Partition2;
class Output2;

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
 placed together.  However we retain the requirement that if two output parts have the same size then
 we will list them in ascending order by input part number and then ascending placement.
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
	
	Weighing2(std::vector<Part>&& provenances) : provenances(std::move(provenances)) {}
	auto operator<=>(const Weighing2&) const = default;
	
	// Accessors
	bool is_symmetric(const Partition2& output) const;
	size_t input_size() const;
	std::vector<uint8_t> input_parts(const Partition2& output_partition) const;
	const Part& provenance(size_t index) const { return provenances[index]; }
	auto begin() const { return provenances.begin(); }
	auto end() const { return provenances.end(); }
	std::vector<uint8_t> pan_contents(const Partition2& output, Placement placement) const;

	void write(Output2& output, const Partition2& output_partition) const;
	static Weighing2* get_instance(std::vector<Part>&& provenances);

private:
	std::vector<Part> provenances;	// Array contains an entry for each part in output partition
		
	/// Static store that owns all weighing objects we have created
	static std::set<std::unique_ptr<Weighing2>, PointerComparator<Weighing2>> cache;
};

#endif /* Weighing2_hpp */

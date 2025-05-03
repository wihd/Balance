//
//  Partition.hpp
//  Balance
//
//  Created by William Hurwood on 4/25/25.
//

#ifndef Partition_hpp
#define Partition_hpp

#include <numeric>
#include <vector>
#include "Types.h"
#include "WeighingIterator.hpp"
class Weighing;

/**
 Immutable class that describes how we have split the coins into parts.
 This class only records the partition - we expect other classes around it to organize the partitions.
 We may assume that every part contains at least one coin, and that there are at least two coins.
 But it is possible that there is only one part in the partition.
 */
class Partition
{
public:
	/// An initial partition has n coins, all in a single group
	Partition(uint8_t count) : parts(1, count) {}
	auto operator<=>(const Partition&) const = default;
	
	/// We can also construct a partition from a Weighing applied to another partition
	Partition(const PartitionProvenance& provenance, const Weighing& weighing, const Partition& base);
	
	/// Return number of coins available in selected part
	uint8_t operator[](size_t i) const { return parts[i]; }
	
	/// Return number of parts in the partition
	size_t size() const { return parts.size(); }
	
	/// Return number of available coins over all of the parts
	uint8_t coin_count() const { return std::reduce(parts.begin(), parts.end()); }
	
	/// Return iterator that generates all weighings for this partition
	WeighingIterator begin() const { return WeighingIterator(this); }
	
	/// Seninel iterator to detect end of weighings for partition
	WeighingIterator end() const { return WeighingIterator(); }

private:
	/// Store number of coins in each part for an implied order of the parts
	std::vector<uint8_t> parts;
};

#endif /* Partition_hpp */

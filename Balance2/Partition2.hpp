//
//  Partition2.hpp
//  Balance2
//
//  Created by William Hurwood on 5/14/25.
//

#ifndef Partition2_hpp
#define Partition2_hpp

#include <vector>
#include <memory>
#include <numeric>
#include <set>
#include "Types2.h"

class Weighing2;
class Output2;

/**
 Class that records a way of partitioning the coins into distinguishable parts.
 At start of problem there will be one part.  Eventually we will have c singleton parts
 which happens once we have performed sufficiently many weighings that all coins have
 been distinguished.
 
 We require: c > 2, p[i]>0, p[i+1] >= p[i].
 
 This class is very similar to Partition.  The primary difference is that we now require
 that the parts to be non-decreasing in size.  This reduces number of distinct partitions we consider.
 
 We have also folded the cache of partitions & weighings into this class.
 Caller may assume that child partitions returned from this class are owned by the cache.
 Thus partitions are equal if and only if they are equal as pointer values.
 */
class Partition2
{
public:
	/// Structure used to record a way of weighing this partition
	struct Child
	{
		Weighing2* weighing;			// Describe the weighing
		Partition2* output;				// The output partition induced by the weighing
	};
	
	/// An initial partition has n coins, all in a single group
	Partition2(uint8_t count) : parts(1, count) {}
	Partition2(std::vector<uint8_t>&& parts) : parts(std::move(parts)) {}
	auto operator==(const Partition2& other) const { return parts == other.parts; }
	auto operator<=>(const Partition2& other) const { return parts <=> other.parts; }
	
	// Accessors
	uint8_t operator[](size_t i) const { return parts[i]; }
	size_t size() const { return parts.size(); }
	uint8_t coin_count() const { return std::reduce(parts.begin(), parts.end()); }
	const std::vector<Child>& get_children();
	static Partition2* get_instance(std::vector<uint8_t>&& parts);

	/// Display a summary of this partition
	void write(Output2& output, const Weighing2* weighing = nullptr) const;

private:
	/// Store number of coins in each part for an implied order of the parts
	std::vector<uint8_t> parts;
	
	/// Once computed, cache the weighings and output partitions that can be obtained from this partition
	/// This data member is not seen as part of the partition and so is ignored for comparison purposes
	std::vector<Child> children;
	
	/// Static store that owns all partition objects we have created
	static std::set<std::unique_ptr<Partition2>, PointerComparator<Partition2>> cache;
};

#endif /* Partition2_hpp */

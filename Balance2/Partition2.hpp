//
//  Partition2.hpp
//  Balance2
//
//  Created by William Hurwood on 5/14/25.
//

#ifndef Partition2_hpp
#define Partition2_hpp

#include <vector>
#include <numeric>

/**
 Immutable class that records a way of partitioning the coins into distinguishable parts.
 At start of problem there will be one part.  Eventually we will have c singleton parts
 which happens once we have performed sufficiently many weighings that all coins have
 been distinguished.
 
 We require: c > 2, p[i]>0, p[i+1] >= p[i].
 
 This class is very similar to Partition.  The primary difference is that we now require
 that the parts to be non-decreasing in size.  This reduces number of distinct partitions we consider.
 */
class Partition2
{
public:
	/// An initial partition has n coins, all in a single group
	Partition2(uint8_t count) : parts(1, count) {}
	auto operator<=>(const Partition2&) const = default;
	
	// Accessors
	uint8_t operator[](size_t i) const { return parts[i]; }
	size_t size() const { return parts.size(); }
	uint8_t coin_count() const { return std::reduce(parts.begin(), parts.end()); }

private:
	/// Store number of coins in each part for an implied order of the parts
	std::vector<uint8_t> parts;
};

#endif /* Partition2_hpp */

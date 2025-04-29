//
//  Weighing.hpp
//  Balance
//
//  Created by William Hurwood on 4/25/25.
//

#ifndef Weighing_hpp
#define Weighing_hpp

#include <vector>
#include <numeric>
#include "Types.h"
class Partition;

/**
 Class representing a single potenial weighing.
 We record how we will select the coins for each pan from a partition.
 The partition is implied.  The outcome of the weighing is not recorded.
 
 To avoid considering the same selection twice, but with the pans reversed we impose the restriction that the
 selection for the right pan must not be lexiographically greater than the selection for the left pan.
 */
class Weighing
{
public:
	Weighing(const Partition* partition);
	auto operator<=>(const Weighing&) const = default;
	
	// Accessors
	auto left_count(size_t index) const { return left[index]; }
	auto right_count(size_t index) const { return right[index]; }
	
	/// The number of coins in each pan in the weighing
	uint8_t pan_count() const { return std::reduce(left.begin(), left.end()); }
	
	/// Describe the partition induced by applying this weighing to a base partition
	PartitionProvenance compute_provenance(const Partition& base) const;
	
	/// Switch to next weighing in standard order on the given partition
	void advance(const Partition& partition);

private:
	/// For each part in partition record the number of coins placed in left pan
	std::vector<uint8_t> left;
	
	/// For each part in partition record the number of coins placed in right pan
	std::vector<uint8_t> right;
	
	// Helper methods
	bool select_right(const Partition& partition);
	bool advance_left(const Partition& partition);
	bool advance_right(const Partition& partition);
	void fill_left(const Partition& partition, uint8_t count, size_t index = 0);
	void fill_right(const Partition& partition, uint8_t count, size_t index = 0);
	void end()
	{
		// We must clear **both** vectors to make this into sentinel value
		// The default <=> operator will compare both arrays to test for end() so we cannot
		// afford to leave one array with content
		left.clear();
		right.clear();
	}
};

#endif /* Weighing_hpp */

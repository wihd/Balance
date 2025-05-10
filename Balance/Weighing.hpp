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
class Output;

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
	
	/// A symmetric weighing is one where swapping right and left pan gives the same weighing
	/// It is important because we only need consider one non-balanced outcome from it
	/// Thus if the problem can be solved from Left and Balanced outcomes then by symmetry it can also
	/// be solved for Right outcomes, so they may be ignored
	bool is_symmetric() const
	{
		// Its easy to recognise a symmetric weighing
		// It happens if and only if both left and right pans take same number of coins from each part
		return left == right;
	}

	/// Switch to next weighing in standard order on the given partition
	void advance(const Partition& partition);
	
	/// Display a summary of this weighing
	void write(Output& output, const Partition& partition) const;

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

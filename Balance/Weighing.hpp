//
//  Weighing.hpp
//  Balance
//
//  Created by William Hurwood on 4/25/25.
//

#ifndef Weighing_hpp
#define Weighing_hpp

#include <vector>
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
	Weighing(const Partition& partition);
	auto operator<=>(const Weighing&) const = default;
	
	/// Switch to next weighing in standard order on the given partition
	void advance(const Partition& partition);

private:
	/// For each part in partition record the number of coins placed in left pan
	std::vector<uint8_t> left;
	
	/// For each part in partition record the number of coins placed in right pan
	std::vector<uint8_t> right;
	
	// Helper methods
	bool advance_left(const Partition& partition);
	void place_left(const Partition& partition, uint8_t count, size_t index = 0);
};

#endif /* Weighing_hpp */

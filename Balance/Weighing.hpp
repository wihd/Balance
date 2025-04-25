//
//  Weighing.hpp
//  Balance
//
//  Created by William Hurwood on 4/25/25.
//

#ifndef Weighing_hpp
#define Weighing_hpp

#include <vector>

/**
 Immutable class representing a single potenial weighing.
 We record how we will select the coins for each pan from a partition.
 The partition is implied.  The outcome of the weighing is not recorded.
 */
class Weighing
{
public:
	auto operator<=>(const Weighing&) const = default;

private:
	/// For each part in partition record the number of coins placed in left pan
	std::vector<uint8_t> mPanLeft;
	
	/// For each part in partition record the number of coins placed in right pan
	std::vector<uint8_t> mPanRight;
};

#endif /* Weighing_hpp */

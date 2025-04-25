//
//  Partition.hpp
//  Balance
//
//  Created by William Hurwood on 4/25/25.
//

#ifndef Partition_hpp
#define Partition_hpp

#include <vector>

/**
 Immutable class that describes how we have split the coins into parts.
 This class only records the partition - we expect other classes around it to organize the partitions.
 */
class Partition
{
public:
private:
	/// Store number of coins in each part for an implied order of the parts
	std::vector<uint8_t> mParts;
};

#endif /* Partition_hpp */

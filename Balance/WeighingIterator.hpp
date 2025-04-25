//
//  WeighingIterator.hpp
//  Balance
//
//  Created by William Hurwood on 4/25/25.
//

#ifndef WeighingIterator_hpp
#define WeighingIterator_hpp

#include <memory>
#include "Weighing.hpp"
class Partition;

/**
 Iterator class used to generate all possible weighings for a partition.
 It will not visit two weighings that differ only by which coins are placed in each pan.
 */
class WeighingIterator
{
public:
	using difference_type = std::ptrdiff_t;	// Concept requirement
	using element_type = Weighing;			// Concept requirement, must be underlying type not const actually returned
	
	/// Construct iterator by specifying partition over which it changes
	WeighingIterator(Partition* ipPartition);

	// Declare methods required by the concept
	WeighingIterator();
	const Weighing& operator*() const { return mCurrent; }
	WeighingIterator& operator++();
	WeighingIterator operator++(int);
	bool operator==(const WeighingIterator& other) const { return mCurrent == other.mCurrent; }
	
private:
	/// The current value of this iterator
	Weighing mCurrent;
	
	/// The partition over whose weighings we are iterating
	Partition* mpPartition;
	// C++20 Note: I wanted to make this a reference
	// But doing so makes it impossible to assign to the class which is required by forward_iterator<> concept
	// So we made it a pointer instead
};


#endif /* WeighingIterator_hpp */

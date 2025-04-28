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
	using element_type = Weighing;			// Concept requirement: we actually return const element_type&
	
	/// Construct iterator by specifying partition over which it changes
	WeighingIterator(Partition* partition);

	// Declare methods required by the concept
	WeighingIterator();
	const Weighing& operator*() const { return current; }
	WeighingIterator& operator++();
	WeighingIterator operator++(int);
	bool operator==(const WeighingIterator& other) const { return current == other.current; }
	
private:
	/// The partition over whose weighings we are iterating (or nullptr for iterator stuck at end())
	Partition* partition;
	// C++20 Note: I originally wanted to make this a reference
	// But doing so makes it impossible to assign to the class which is required by forward_iterator<> concept
	// So we made it a pointer instead

	/// The current value of this iterator
	Weighing current;
};

#endif /* WeighingIterator_hpp */

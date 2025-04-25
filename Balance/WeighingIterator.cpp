//
//  WeighingIterator.cpp
//  Balance
//
//  Created by William Hurwood on 4/25/25.
//

#include "WeighingIterator.hpp"

WeighingIterator::WeighingIterator()
{
	// The forward_iterator concept requires that we provide a default constructor
	// But since class must have a partition pointer we cannot implement it
	throw std::runtime_error("Not implemented WeighingIterator forward constructor");
}

WeighingIterator::WeighingIterator(Partition* ipPartition) : mpPartition(ipPartition)
{
	
}

WeighingIterator& WeighingIterator::operator++() {
	return *this;
}

WeighingIterator WeighingIterator::operator++(int) {
	WeighingIterator old_value = *this;
	operator++();
	return old_value;
}

// C++20 Note: Tell compiler that this class is intended to satisfy constraints of forward iterator
static_assert(std::forward_iterator<WeighingIterator>);

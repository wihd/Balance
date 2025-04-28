//
//  WeighingIterator.cpp
//  Balance
//
//  Created by William Hurwood on 4/25/25.
//
#include <cassert>
#include "WeighingIterator.hpp"
#include "Weighing.hpp"
#include "Partition.hpp"

WeighingIterator::WeighingIterator(): partition(nullptr), current(nullptr)
{
	// The forward_iterator concept requires that we provide a default constructor
	// This is supported - it will make an iterator that matches the end() position
}

WeighingIterator::WeighingIterator(Partition* input) : partition(input), current(input)
{
	// This is the real constructor
}

WeighingIterator& WeighingIterator::operator++() {
	// Actually the advance function is implemented within the Weighing class
	assert(partition != nullptr);
	current.advance(*partition);
	return *this;
}

WeighingIterator WeighingIterator::operator++(int) {
	WeighingIterator old_value = *this;
	operator++();
	return old_value;
}

// C++20 Note: Tell compiler that this class is intended to satisfy constraints of forward iterator
static_assert(std::forward_iterator<WeighingIterator>);

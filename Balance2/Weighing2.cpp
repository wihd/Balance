//
//  Weighing2.cpp
//  Balance2
//
//  Created by William Hurwood on 5/14/25.
//

#include "Weighing2.hpp"

// Initialise static values
decltype(Weighing2::cache) Weighing2::cache{};


Weighing2* Weighing2::get_instance(std::vector<Part>&& provenances)
{
	// Get the cached instance of a weighing with the given provenances, creating it if necessary
	Weighing2 probe(std::move(provenances));
	auto iterator = cache.find(&probe);
	if (iterator != cache.end())
	{
		// Switch to partition instance already in cache
		return iterator->get();
	}
	else
	{
		// Create and return a cached instance of the partition
		auto instance = std::make_unique<Weighing2>(std::move(probe));
		auto result = instance.get();
		cache.insert(std::move(instance));
		return result;
	}
}

//
//  Partition2.cpp
//  Balance2
//
//  Created by William Hurwood on 5/14/25.
//

#include <cassert>
#include <algorithm>
#include "Partition2.hpp"
#include "Weighing2.hpp"

// Initialise static values
decltype(Partition2::cache) Partition2::cache{};

// We use a local generator class to track the creation of weighings below a partition
class Generator
{
public:
	Generator(const Partition2& input);
	Partition2::Child get() const;

private:
	const Partition2& input;				// The partition whose weighings we are computing
	std::vector<uint8_t> left;				// For each input part, the number of coins placed in left pan
	std::vector<uint8_t> right;				// For each input part, the number of coins placed in right pan
};

Generator::Generator(const Partition2& input) :
	input(input),
	left(input.size()),
	right(input.size())
{
	// The first weighing selects one coin for each pan, taking it from earlier available part
	assert(input.coin_count() >= 2);
	left[0] = 1;
	if (input[0] >= 2)
	{
		right[0] = 1;
	}
	else
	{
		right[1] = 1;
	}
}

Partition2::Child Generator::get() const
{
	// Under assumption that caller already knows the generator has another weighing
	// report the value of that weighing
	
	// The natural order that we generate output parts is not the order we are required to use
	// We will accumulate the data in a local structure so its easier to sort
	struct Item
	{
		int input_part;				// Input partition part from which this output part is refined
		Placement placement;		// Pan into which this set of coins is placed
		uint8_t part_size;			// Number of coins in this output part
		
		bool operator<(const Item& other) const
		{
			// Sort by part_size then input_part then placement
			if (part_size != other.part_size)
			{
				return part_size < other.part_size;
			}
			if (input_part != other.input_part)
			{
				return input_part < other.input_part;
			}
			return placement < other.placement;
		}
	};

	// Each input part will yield between one and three output parts
	// Compute the provanences and input part sizes in order of output
	std::vector<Item> items;
	for (size_t i = 0; i != input.size(); ++i)
	{
		auto count = input[i];
		if (left[i] > 0)
		{
			items.emplace_back(i, Placement::LeftPan, left[i]);
			count -= left[i];
		}
		if (right[i] > 0)
		{
			items.emplace_back(i, Placement::RightPan, right[i]);
			count -= right[i];
		}
		if (count > 0)
		{
			items.emplace_back(i, Placement::SetAside, count);
		}
	}

	// Sort the items to favour the partition size ordering over input item part number
	std::sort(items.begin(), items.end());
	
	// Separate the partition and weighing information
	std::vector<uint8_t> output_parts;
	std::vector<Weighing2::Part> provenances;
	output_parts.reserve(items.size());
	for (auto& item : items)
	{
		output_parts.push_back(item.part_size);
		provenances.emplace_back(item.input_part, item.placement);
	}
	
	// Switch to a cached instance of each object so we only have one of each
	return
	{
		Weighing2::get_instance(std::move(provenances)),
		Partition2::get_instance(std::move(output_parts))
	};
}

const std::vector<Partition2::Child>& Partition2::get_children()
{
	// We are asked to give the list of children (i.e. distinct weighings) of this partition
	// We only compute this once.  Since it is never empty we can determine if we computed it before
	if (children.empty())
	{
		// We need a generator class to track the effort of computing the weighings
		Generator generator(*this);
	}
	
	return children;
}

Partition2* Partition2::get_instance(std::vector<uint8_t>&& parts)
{
	// Get the cached instance of a partition with the given parts, creating it if necessary
	Partition2 probe(std::move(parts));
	auto iterator = cache.find(&probe);
	if (iterator != cache.end())
	{
		// Switch to partition instance already in cache
		return iterator->get();
	}
	else
	{
		// Create and return a cached instance of the partition
		auto instance = std::make_unique<Partition2>(std::move(probe));
		auto result = instance.get();
		cache.insert(std::move(instance));
		return result;
	}
}

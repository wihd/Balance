//
//  Weighing2.cpp
//  Balance2
//
//  Created by William Hurwood on 5/14/25.
//

#include <format>
#include "Weighing2.hpp"
#include "Partition2.hpp"
#include "Output2.hpp"

// Initialise static values
decltype(Weighing2::cache) Weighing2::cache{};

size_t Weighing2::input_size() const
{
	// Compute the part sizes of the input partition of this weighing
	// Probably anyone who has access to this weighing also has access to its input partition
	// But if we only need to compute them once we can deduce them from the information contained here
	// (In contrast the output partition cannot be deduced)
	size_t max_part = 0;
	for (auto& p : provenances)
	{
		if (p.part > max_part)
		{
			max_part = p.part;
		}
	}
	return max_part + 1;
}

std::vector<uint8_t> Weighing2::input_parts(const Partition2& output_partition) const
{
	// Compute the part sizes of the input partition of this weighing
	// Probably anyone who has access to this weighing also has access to its input partition
	// But if we only need to compute them once we can deduce them from the information contained here
	// (In contrast the output partition cannot be deduced)
	std::vector<uint8_t> result(input_size(), 0);
	for (size_t i = 0; i != provenances.size(); ++i)
	{
		result[provenances[i].part] += output_partition[i];
	}
	return result;
}

std::vector<uint8_t> Weighing2::pan_contents(const Partition2& output,
											 Placement placement) const
{
	// Compute the number of coins from each input part that are placed in the given pan
	// If we need to create this information regularly we could cache it within the weighing
	// Indeed in the earlier balance that's what we did
	// But it seems we only used this information for writing the output we will not store it
	// and instead compute it when needed
	std::vector<uint8_t> result(input_size(), 0);
	for (size_t i = 0; i != provenances.size(); ++i)
	{
		auto& p = provenances[i];
		if (p.placement == placement)
		{
			result[p.part] += output[i];
		}
	}
	return result;
}

std::string write_pan_description(const std::vector<uint8_t>& content, const std::vector<uint8_t>& input)
{
	std::string result;
	for (size_t i = 0; i != content.size(); ++i)
	{
		// Do not mention parts with no contribution
		if (content[i] == 0)
		{
			continue;
		}
		if (!result.empty())
		{
			result += "+";
		}

		if (content[i] == input[i])
		{
			// Special case for when we send entire part
			result += std::format("p[{}]", i);
		}
		else
		{
			// Specify how much of input part 
			result += std::format("p[{}]({}/{})", i, content[i], input[i]);
		}
	}
	return result.empty() ? "Empty" : result;
}



void Weighing2::write(Output2& output, const Partition2& output_partition) const
{
	// Although the weighing is storing the provanence of each output partition
	// we think it makes more sense to the reader to describe how much of each input part is assigned
	// to each placement.
	
	// We can compute input partition rather than requiring caller to pass it to us
	auto input = input_parts(output_partition);
	output.println("Weighing:  {{ Left: {};  Right: {};  Aside: {} }}",
				   write_pan_description(pan_contents(output_partition, Placement::LeftPan), input),
				   write_pan_description(pan_contents(output_partition, Placement::RightPan), input),
				   write_pan_description(pan_contents(output_partition, Placement::SetAside), input));
}

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


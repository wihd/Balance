//
//  Partition.cpp
//  Balance
//
//  Created by William Hurwood on 4/25/25.
//
#include <cassert>
#include <format>
#include "Partition.hpp"
#include "Weighing.hpp"
#include "Output.hpp"

Partition::Partition(const PartitionProvenance& provenance, const Weighing& weighing, const Partition& base)
{
	// The provenance (which is in effect a partial computation) tells us how many parts we need
	// But to compute the size of each part we must examine the weighing
	parts.reserve(provenance.size());
	int base_part = -1;
	uint8_t base_count = 0;
	for (auto& part_provenance : provenance)
	{
		// We may assume that parts from same base part are adjacent & end with the coins set aside
		// So we can compute the number of coins set aside as we go along
		if (base_part != part_provenance.part)
		{
			base_part = part_provenance.part;
			base_count = base[base_part];
		}
		
		switch (part_provenance.placement)
		{
			case Placement::LeftPan:
				parts.push_back(weighing.left_count(base_part));
				base_count -= weighing.left_count(base_part);
				break;
			case Placement::RightPan:
				parts.push_back(weighing.right_count(base_part));
				base_count -= weighing.right_count(base_part);
				break;
			case Placement::SetAside:
				parts.push_back(base_count);
				break;
		}
	}
	
	// Both partitions should have same size
	assert(coin_count() == base.coin_count());
}

void Partition::write(Output& output, const PartitionProvenance* provenance) const
{
	// Optionally caller may provide a provenance vector
	if (provenance)
	{
		// Each part consists of the members of some input part that were sent to some bucket
		// Count for each input part how many ways it is divided
		// We know there cannot be more input parts than output parts
		std::vector<int> counters(parts.size());
		for (auto& p : *provenance)
		{
			counters[p.part] += 1;
		}
		
		std::vector<std::string> part_provenances;
		for (auto& p : *provenance)
		{
			if (counters[p.part] == 1)
			{
				part_provenances.push_back(std::format("p[{}]", p.part));
			}
			else
			{
				part_provenances.push_back(std::format("p[{}]@{}", p.part, placement_names[p.placement]));
			}
		}

		// C++ 20 Note: The {::s} format makes it omit string delimiters around the parts
		output.println("Partition: {{ {} part{};  Sizes: {};  Provenances: {::s} }}",
					   parts.size(), parts.size() == 1 ? "" : "s", parts, part_provenances);
	}
	else
	{
		// C++20 Note: Built in support for formatting vectors (and other ranges)
		output.println("Partition: {{ {} part{};  Sizes: {} }}",
					   parts.size(), parts.size() == 1 ? "" : "s", parts);
	}
}

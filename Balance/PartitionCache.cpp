//
//  PartitionCache.cpp
//  Balance
//
//  Created by William Hurwood on 4/30/25.
//
#include <cassert>

#include "PartitionCache.hpp"

const Partition* PartitionCache::get_root(uint8_t coin_count)
{
	// The roots map acts as an index to root partitions
	auto& root = roots[coin_count];
	if (root == nullptr)
	{
		// Construct the root partition and store it
		auto new_root = std::make_unique<Partition>(coin_count);
		root = new_root.get();
		cache.emplace(std::move(new_root), Item{});
	}
	return root;
}

const PartitionCache::Item& PartitionCache::get_weighings(const Partition* partition)
{
	// Look up the entry for this partition (we expect it to already exist)
	auto entry = cache.find(partition);
	assert(entry != cache.end());
	
	// If we have not already computed the weighings for this partition, compute them now
	auto& item = entry->second;
	if (item.weighings.empty())
	{
		// Iterate over all wieighings for the partition
		for (auto& weighing : *partition)
		{
			// Store the weighing, switching to instance belonging to cache
			auto weighing_entry = weighings_cache.find(&weighing);
			if (weighing_entry == weighings_cache.end())
			{
				// Cache a copy of this weighing to use within this execution
				auto new_weighing = std::make_unique<Weighing>(**weighing_entry);
				item.weighings.push_back(new_weighing.get());
				weighings_cache.insert(std::move(new_weighing));
			}
			else
			{
				// Cache already has equivalent weighing object
				item.weighings.push_back(weighing_entry->get());
			}
			
			// Construct the corresponding partition
			auto provenance = weighing.compute_provenance(*partition);
			auto new_partition = std::make_unique<Partition>(provenance, weighing, *partition);
			
			// Although the provance object is easy to reconstruct from weighing and parent partition
			// it seems to be more useful than the weighing when considering problems
			// So we will cache it also within out items structure
			auto provenance_entry = provanences_cache.find(&provenance);
			if (provenance_entry == provanences_cache.end())
			{
				// Unlike the weighing (which belongs to the parent partition and so must be copied)
				// we are done with using the provance here, so we can move its value into value owned
				// by a strong pointer, which will then move into our cache
				auto unique_provanence = std::make_unique<PartitionProvenance>(std::move(provenance));
				item.provenances.push_back(unique_provanence.get());
				provanences_cache.insert(std::move(unique_provanence));
			}
			else
			{
				// Cache already has equivalent provenance object
				item.provenances.push_back(provenance_entry->get());
			}

			// Use the cached pointer not the one we constructed for the child partition
			auto emplace_result = cache.try_emplace(std::move(new_partition), Item{});
			item.partitions.push_back(emplace_result.first->first.get());
		}
	}
	return item;
}

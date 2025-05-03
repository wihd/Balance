//
//  PartitionCache.hpp
//  Balance
//
//  Created by William Hurwood on 4/30/25.
//

#ifndef PartitionCache_hpp
#define PartitionCache_hpp

#include <memory>
#include <vector>
#include <map>
#include <set>
#include "Partition.hpp"
#include "Weighing.hpp"

/**
 Partition cache is used to own the partition objects that we have created.
 
 We expect that we will often encounter the same partition in different branches.
 We would prefer not to store multiple copies of the partition.
 Furthermore the set of available weighings for a partition do not depend on the provanence of each part.
 So this class cache's all of the partitions we have considered, and possibly their child partitions.
 
 Using this class also allows us to refer to partitions (and weighings) by pointer.
 For the time being we will not attempt to reference count partitions.
 */
class PartitionCache
{
public:
	/// The information we store about weighings from a parition
	struct Item
	{
		std::vector<const Weighing*> weighings;		// If computed the weighings applied to this partition
		std::vector<const Partition*> partitions;	// If computed the outcome from each weighing
	};
	
	/// Return a root partition (all coins in same part)
	const Partition* get_root(uint8_t coin_count);
	
	/// Return, computing if necessary, the weighings and returning partitions for a given partition
	const Item& get_weighings(const Partition* partition);
	
private:
	// Compare various kinds of partitions
	template <class T>
	struct PointerComparator
	{
		// Specify that we support comparison of various options
		using is_transparent = std::true_type;

		bool operator()(const std::unique_ptr<const T>& a, const std::unique_ptr<const T>& b) const
		{ return *a < *b; }
		
		bool operator()(const std::unique_ptr<const T>& a, const T* b) const
		{ return *a < *b; }

		bool operator()(const T* a, const std::unique_ptr<const T>& b) const
		{ return *a < *b; }
	};
	
	/// The keys of this map own the partition objects, the values are corresponding weighings if computed
	std::map<std::unique_ptr<const Partition>, Item, PointerComparator<Partition>> cache;
	
	/// Map giving root partitions we have made earlier (they are owned by cache)
	std::map<uint8_t, const Partition*> roots;
	
	/// Set that owns our weighings - we expect the same weighing to occur in multiple partitions
	std::set<std::unique_ptr<const Weighing>, PointerComparator<Weighing>> weighings_cache;
};

#endif /* PartitionCache_hpp */

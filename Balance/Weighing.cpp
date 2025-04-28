//
//  Weighing.cpp
//  Balance
//
//  Created by William Hurwood on 4/25/25.
//
#include <cassert>
#include "Weighing.hpp"
#include "Partition.hpp"

/*
 When generating all of the possible weighings for a partition, we put them into following order:
 1. We select weighings with N coins per pan, N=1, 2, ...,
 2. We select content of left pan, making the lexiographically biggest selection first and then stepping down
 3. We select right pan as lexiographically largest subject to phyiscal constaint (total coins selected by
    both pans for a part cannot exceed number of coins in part) and lexical constraint (right pan must not
    be lexically greater than left pan).
 
 One optimisation we use is that once we are unable to find any selection for right pan for a given left pan
 we abort decrementing the content of the left pan.  We have proven that any lexiographically smaller selection
 cannot itself have any solutions so there is no point in looking further.  But the proof is lengthy and we
 suspect that there must be a simpler way of doing it.
 
 Let p_1, p_2, ..., p_n be the partition, defined as number of coins in each part.
 Let x_1, x_2, ..., x_n be the number of coins selected from each part for left pan
 Let N = \sum x_i, the number of coins in left part
 
 We will compute a selection y_1, y_2, ,,,, y_n for right pan subject to:
 - physical restriction: x_i + y_i <= p_i
 - lexical restriction: \vec{y} is lexiographically smaller than \vec{x}
 - maximum size: \sum y_i will be as large as possible when obeying first two restrictions
 
 Let Y = \sum y_i.  Its easy to see that a valid selection for right pan will exist if and only if Y >= N.
 Out claim is that if we decrement \vec{x} then Y will always be unchanged, or decrease.  Thus once Y < N it will
 remain true for all subsequent \vec{x}.
 
 Let us compute Y.  First we observe that if there are any leading i where x_i = 0, then y_i = 0 as well.
 This follows from requirement that \vec{y} is not lexically greater than \vec{x}.
 Let j be smallest index s.t. x_j > 0.  Then y_1, ...., y_{j-1} = 0.
 
 Case A:
 Suppose 2x_j > p_j.  Then the phyisical constraint forces y_j < x_j, which in turn ensures that \vec{y} is lex
 smaller than \vec{x}.  So in this case we can select as many coins as are allowed for every part after j.
 Y = p_j - x_j + \sum_{i > j}(p_i - x_i)
   = p_j - x_j + \sum_{i > j}(p_i) - N + x_j
   = p_j + \sum_{i > j}(p_i) - N
 Observe that as we decrment \vec{x} we will not change \vec{p} or N.  So this expressin depends only on j which
 can only increase.  So this formula for Y must decrement as we decrease \vec{x}.
 
 Case B:
 Suppose that 2x_j <= p_j.  Now look at each i in turn with i > j.  If 2x_i = p_i (which means that both lexical
 and physical constaints want to set y_i=x_i) we increment i.  Suppose that either we reach the end of the vector
 or we find some k with 2x_k > p_k.  Then we are in case B.
 
 For this case we can maximise Y by using largest possible lexical constain for i = 1, ..., j, ..., k-1.
 That means that for i < k, we select y_i = x_i.  But for j < i < k this is same as selecting y_i = p_i - x_i.
 For i=k we must obey the phyisical constaint which forces y_k < x_k.
 For i > k we select y_i = p_i - x_i.
 Y = x_j + \sum_{i > j}(p_i - x_i)
   = x_j + \sum_{i > j}(p_i) - N + x_j
   = 2x_j + \sum_{i > j}(p_i) - N
 When we decement \vec{x} without changing j or x_j then this formula is unchanged.
 When we fix j but decrement x_j this formula must shrink.
 
 Case C:
 Otherwise we have 2x_j <= p_j and k exists with 2x_k < p_k.  In this case we will be able to select more coins
 from part k if we have already satisified the lex constraint by selecting fewer coins for right pan some earlier part
 then were selected for left pan.  It does not matter which earlier part, so to make the formulae match we select
 one fewer coin from part j than was lex permitted.  This will allow us to select more coins for part k (and for
 all subsequent parts).  So the formula for Y is almost identical.
 Y = x_j - 1 + \sum_{i > j}(p_i - x_i)
   = 2x_j - n + \sum_{i > j}(p_i) - N
 
 Each case individually decreases as we decrement \vec{x}.  We also observe that the difference between the formula
 is only in first term:
 - A:  p_j
 - B:  2x_j
 - C:  2x_j - 1
 
 Consider what happens as we decrement \vec{x} without changing j.  First x_j will be maximal.
 As long as 2x_j > p_j we will stay in case A, and the value for Y will be unchanged.
 
 Eventually we will decrease x_j so 2x_j <= p_j.  We will then enter case B or C, and remain within them as long
 as j is unchanged.  Observe that whichever of these cases we are in the value of Y cannot be greater than it
 was in case A.  So this transition cannot make Y increase.
 
 As we decrement \vec{x} we will decrease x_j.  We claim that for each x_j it will first be in case B, then move
 to case C (which deccreases Y).  When x_j decreases it will jump back to case B, but since the leading term is
 2x_j the minus one will not matter and Y will continue to decrease.  This claim is true because every \vec{x}
 with fixed x_j in case B is lex bigger than \vec{x} with same x_j in case C.
 
 Once we increase j we can see that the formulae can only decrease.  N remains fixed.  An increase in j removes
 a term of size p_{j+1} from \sum_{i > j}(p_i).  But the larset possible value of Y (case A) uses p_{j+1} as its
 leading term.  So Y cannot increase.
 
 This proves the result.
 */

Weighing::Weighing(const Partition* partition) :
	left(partition ? partition->size() : 0),
	right(partition ? partition->size() : 0)
{
	// Caller can construct with no partition if caller wants to make a sentinel value
	if (partition)
	{
		// The first weighing selects one coin for each pan
		assert(partition->coin_count() >= 2);
		left[0] = 1;
		if ((*partition)[0] >= 2)
		{
			right[0] = 1;
		}
		else
		{
			right[1] = 1;
		}
	}
}

void Weighing::advance(const Partition& partition)
{
	// We assume that this weighing is already a valid weighing for the partition
	// Switch to the next weighing in the standard order
	
	// We use a weighing with no entries as a sentinel value for end of sequence
	// Attempt to find another weighing by changing the right pan, keeping left pan fixed
	if (!left.empty() && !advance_right(partition))
	{
		// There were no more options to advance right pan, so advance left pan instead
		if (!advance_left(partition))
		{
			// Since we could not make a different selection for left pan, it is impossible to make another
			// selection for entire weighing.  Switch to the sentinel value.
			end();
		}
		else if (!select_right(partition))
		{
			// The large comment above justifies the claim that if it is impossible to select right
			// lexiographically smaller than left (we know that there are enough coins if we had a free selection)
			// then it will be impossible for any left lexiographically smaller than its current value
			// So we should go directly to case where we start with a larger selection
			uint8_t new_pan_count = pan_count() + 1;
			if (2 * new_pan_count > partition.coin_count())
			{
				// There are not enough coins to use selections of this size
				end();
			}
			else
			{
				// Set left to be the first coins - it has to be possible to fill on right
				fill_left(partition, new_pan_count);
				select_right(partition);
			}
		}
	}
}

bool Weighing::select_right(const Partition& partition)
{
	// Try to select coins for right pan consistent with left pan selection
	// We do not assume that right[] is set to all zeros on entry
	
	// First try to set every part in right to same in left
	uint8_t count = pan_count();
	for (size_t index = 0; index != right.size(); ++index)
	{
		if (partition[index] >= 2 * left[index])
		{
			right[index] = std::min(count, left[index]);
			count -= right[index];
		}
		else
		{
			// We cannot match left selection for this part, since there were too few coins left in the part
			right[index] = std::min(count, static_cast<uint8_t>(partition[index] - left[index]));
			count -= right[index];
			
			// Now that we have selected fewer coins at index we can select maximal available coins in all
			// other parts without violating lexical constraint
			// We do not use fill_right() since we do not know that we will be able to select count coins
			for (auto i = index + 1; i != right.size(); ++i)
			{
				right[i] = std::min(count, static_cast<uint8_t>(partition[i] - left[i]));
				count -= right[i];
			}
			
			// If we were able to select all coins then this must be lexiographically first selection
			if (count == 0)
			{
				return true;
			}
			
			// The largest possible (lexiographically) selection did not work
			// However a better selection might be possible.  There may be some i (i < index) where
			// we could select one fewer coins.  This might allow us to select more for later indexes
			// because we would satisfy the lexical concern earlier
			++count;

			// Search back looking for indexes where we could have choosen more
			// NB There is no point in looking at index==0 since we need to have an earlier part to decrement
			while (index > 1)
			{
				// Previously we selected right[index] coins here
				// But without lexiographical constraint we could have selected partition[index] - left[index]
				--index;
				count += right[index];
				right[index] = std::min(count, static_cast<uint8_t>(partition[index] - left[index]));
				count -= right[index];
				
				// If we have reduced count to zero we have a chance
				if (count == 0)
				{
					// If we can remove one from an index before current index then we will have a solution
					while (index > 0)
					{
						if (right[--index] > 0)
						{
							// Reduce size of selection for this part
							--right[index];

							// However the solution could easily be wrong because we added coins in wrong order
							// (To be lex maximal we should have added in order of incrementing index)
							// We know count up this index is not enough (since right[i] <= left[i] in this range)
							count = pan_count();
							for (size_t i = 0; i <= index; ++i)
							{
								count -= right[i];
							}
							assert(count >= 1);
							
							// Now we fill again.  The point is that because we placed some coins earlier
							// this fill will run out of coins before it reaches the end
							fill_right(partition, count, index + 1);
							return true;
						}
					}
					
					// We must have used the first non-zero part to get enough coins
					// This means that we cannot find a selection that satisfies the lexiograph restriction
					return false;
				}
			}
			return false;
		}
	}
	
	// Identical selection works - it must be the lexically largest permitted selection
	return true;
}

bool Weighing::advance_left(const Partition& partition)
{
	// We assume that left contains a valid selection.  In addition to satisfying physical constaint
	// it is known that it is possible to make a valid selection for right pan as well.
	// We change the left vector to a different valid selection, returning false if this cannot be done.
	
	// Initially we keep the current number of coins in the pan fixed, but try selecting them from a different
	// part of the partition.  The new selection will be lexiographically smaller.
	
	// We start by searching from back of vector looking for a part where there is room to increase the number
	// of coins selected.  We count the number of coins that we have passed over
	uint8_t count = 0;
	size_t index = left.size() - 1;
	while (left[index] == partition[index])
	{
		count += left[index];
		
		// The input assumption that it is possible to make a valid assumption for right pan implies
		// that there must be a part for which left pan did not select all coins in partition
		// So we should not need to check at run time that index has not underflowed
		assert(index > 0);
		--index;
	}
	count += left[index];
	
	// Now we search for an earlier part that has selected some coins in left pan
	// It is possible that no such part exists
	while (index > 0)
	{
		--index;
		if (left[index] > 0)
		{
			// We advance left pan by decrementing this slot, and selecting coins as early as possible after it
			// Since we already passed a slot with spare capacity we must be able to place count coins
			--left[index];
			fill_left(partition, count + 1, index + 1);
			return true;
		}
	}
	
	// There are no more selections for left with the same size
	// count must have cleared all of the previous coins so it tells us how many coins were initially in the left pan
	// We can try again with a larger selection, but we will only do so if there are enough coins in partition
	// to permit this.
	++count;
	if (2 * count > partition.coin_count())
	{
		return false;
	}
	
	// Try again with the additional coins
	fill_left(partition, count);
	return true;
}

bool Weighing::advance_right(const Partition& partition)
{
	// Assume that on entry left and right are both set to valid selections
	// Advance right to the next entry which is lexiographically smaller than its current entry but which uses
	// same number of coins and respects coins already taken by left
	// Return true if this can be done
	// Note that we do not need to do anything to ensure that the new right selection is leciographically
	// smaller than left selection.  It must be true by transitivity.

	// Search for last part with capacity to take more coins
	uint8_t count = 0;
	size_t index = right.size() - 1;
	while (right[index] + left[index] == partition[index])
	{
		count += right[index];

		// Unlike left pan (where we know there is spare capacity for right pan) it is possible that there is no
		// space capacity.  This would happen if the weighing includes all coins in the partition
		if (index == 0)
		{
			// Since there is no spare capacity there are no other ways to select right pan's content
			return false;
		}
		--index;
	}
	count += right[index];
	
	// Now we search for an earlier part that has selected some coins in right pan
	// It is possible that no such part exists
	while (index > 0)
	{
		--index;
		if (right[index] > 0)
		{
			// We advance right pan by decrementing this slot, and selecting coins as early as possible after it
			// Since we already passed a slot with spare capacity we must be able to place count coins
			--right[index];
			fill_right(partition, count + 1, index + 1);
			return true;
		}
	}
	
	// There are no more options for right pan with this number of coins
	// Since both pans must have same number of coins we give up here (advance_left() tries again with more coins)
	return false;
}

void Weighing::fill_left(const Partition& partition, uint8_t count, size_t index)
{
	// Place `count` coins in left pan, starting from part `index`
	// We will make no assumptions about existing values in these slots
	// We expect that caller will have validated that there is room for these coins
	for (; index != left.size(); ++index)
	{
		// Select as many coins as possible
		left[index] = std::min(count, partition[index]);
		count -= left[index];
	}
	assert(count == 0);
}

void Weighing::fill_right(const Partition& partition, uint8_t count, size_t index)
{
	// Place count coins in right pan, starting from index+1 part
	// Caller already knows that there is room to place the coins
	for (; index != right.size(); ++index)
	{
		// Select as many coins as possible
		// C++ note: To avoid integer type promotion we must explicitly use uint8_t for this expression
		right[index] = std::min(count, static_cast<uint8_t>(partition[index] - left[index]));
		count -= right[index];
	}
	assert(count == 0);
}

//
//  Types2.h
//  Balance
//
//  Created by William Hurwood on 5/14/25.
//

// Declare some types and enumerations that we want to use within entire application

#ifndef Types2_h
#define Types2_h

/** Enumerate the options for handling a coin in a weighing */
typedef enum : int {
	LeftPan,
	RightPan,
	SetAside,
} Placement;

/** Transparaent comparator class that allows mixing unique and raw pointers */
template <class T>
struct PointerComparator
{
	// Comparator supports comparing types other than the main type
	using is_transparent = std::true_type;

	// Provide comparators for the three cases we expect to occur
	// Since the container should be using unique pointers we do not need to compare raw pointers
	bool operator()(const std::unique_ptr<T>& a, const std::unique_ptr<T>& b) const { return *a < *b; }
	bool operator()(const std::unique_ptr<T>& a, const T* b) const { return *a < *b; }
	bool operator()(const T* a, const std::unique_ptr<T>& b) const { return *a < *b; }
};


#endif /* Types2_h */

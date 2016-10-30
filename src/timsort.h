#include <iostream>

enum EWhatMerge {
	WM_NoMerge,
	WM_MergeXY,
	WM_MergeYZ
};

class ITimSortParams {
public:
	virtual unsigned int minRun(unsigned int n) const = 0;
	virtual bool needMerge(unsigned int lenX, unsigned int lenY) const = 0;
	virtual EWhatMerge whatMerge(unsigned int lenX, unsigned int lenY, unsigned int lenZ) const = 0;
	virtual unsigned int GetGallop() const = 0;

	virtual ~ITimSortParams() {};
};

#include "timsort-internal.h"

template <class RandomAccessIterator, class Compare>
void TimSort(RandomAccessIterator first, RandomAccessIterator last, const Compare& comp, const ITimSortParams& params) {
	TimSortController<RandomAccessIterator, Compare>::sort(first, last, comp, params);
}
template <class RandomAccessIterator, class Compare>
void TimSort(RandomAccessIterator first, RandomAccessIterator last, const Compare& comp) {
	TimSortController<RandomAccessIterator, Compare>::sort(first, last, comp);
}
template <class RandomAccessIterator>
void TimSort(RandomAccessIterator first, RandomAccessIterator last, const ITimSortParams& params) {
	TimSortController<RandomAccessIterator>::sort(first, last, params);
}
template <class RandomAccessIterator>
void TimSort(RandomAccessIterator first, RandomAccessIterator last) {
	TimSortController<RandomAccessIterator>::sort(first, last);
}


#include <vector>
#include <functional>
#include <string>
#include <sstream>


class SortTestResult {
public:
	const std::vector<unsigned int> crashIndeces;
	const bool success;
	const unsigned long long time;

	SortTestResult(unsigned long long time, std::vector<unsigned int> crashIndeces)
		:crashIndeces(crashIndeces), success(crashIndeces.empty()), time(time)
	{}

	const std::string toString() const {
		std::basic_ostringstream<char> out;
		if (success) {
			out << "Test succeed; time: "  << time;
		} else {
			out << "Test crashed (" << crashIndeces.size() << " wrong elements); time: " << time;
		}
		return out.str();
	}
};

template <class ElementType, class ContainerIterator>
class ContainerAllocator {
public:
	typedef ContainerIterator Iterator;

	virtual const ContainerIterator begin() = 0;
	virtual const ContainerIterator end() = 0;

	virtual ~ContainerAllocator() {}
};

template <class ElementType, class ContainerAllocatorSpecial, class Comparator = std::less<ElementType>>
class SortTest {
private:
	std::vector<ElementType> elements;
	unsigned int size;
	const Comparator comparator;

	typedef typename ContainerAllocatorSpecial::Iterator IteratorType;

public:
	SortTest(ElementType* data, unsigned int size, const Comparator& comparator = Comparator())
		:elements(size), size(size), comparator(comparator) {
		for (unsigned int i = 0; i < size; ++i)
			elements[i] = data[i];
	}

	template <class SortAlgorithm, class Params = void>
	SortTestResult applyTest(const SortAlgorithm& sorter, const Params* params = nullptr) {
		ContainerAllocator<ElementType, IteratorType>* const allocator = allocateInstance();

		unsigned long long workTime = clock();

		runSort(allocator->begin(), allocator->end(), sorter, params);

		workTime = (clock() - workTime) * 1000L / CLOCKS_PER_SEC; // in ms
		std::vector<unsigned int> crashIndeces = findCrashIndeces(allocator->begin(), allocator->end());

		delete allocator;
		return SortTestResult(workTime, crashIndeces);
	}

	ContainerAllocator<ElementType, IteratorType>* const allocateInstance() const {
		return new ContainerAllocatorSpecial(elements);
	}

private:

	std::vector<unsigned int> findCrashIndeces(IteratorType begin, IteratorType end) {
		std::vector<unsigned int> res;

		if (begin == end)
			return res;

		unsigned int index = 0;

		while (++begin < end) {
			if (comparator(begin[0], begin[-1]))
				res.push_back(index);

			++index;
		}

		return res;
	}

	template <class SortAlgorithm, class Params>
	void runSort(IteratorType begin, IteratorType end, const SortAlgorithm& algo, const Params* params) {
		algo(begin, end, comparator, *params);
	}

	template <class SortAlgorithm>
	void runSort(IteratorType begin, IteratorType end, const SortAlgorithm& algo, const void* params) {
		algo(begin, end, comparator);
	}
};

template <class ElementType>
class ArrayAllocator: public ContainerAllocator<ElementType, ElementType*> {
private:
	ElementType* data;
	const unsigned int size;

	ArrayAllocator<ElementType>& operator =(const ArrayAllocator&);
	ArrayAllocator(const ArrayAllocator&);

public:
	ArrayAllocator(std::vector<ElementType> els)
		:data(new ElementType[els.size()]), size(els.size())
	{
		for (unsigned int i = 0; i < size; ++i) {
			data[i] = els[i];
		}
	}

	ElementType* const begin() {
		return data;
	}
	ElementType* const end() {
		return data + size;
	}

	~ArrayAllocator() {
		delete[] data;
	}
};

template <class ElementType>
class VectorAllocator: public ContainerAllocator<ElementType, typename std::vector<ElementType>::iterator> {
private:
	std::vector<ElementType> data;

public:
	VectorAllocator(const std::vector<ElementType>& els)
		:data(els)
	{}

	const typename std::vector<ElementType>::iterator begin() {
		return data.begin();
	}
	const typename std::vector<ElementType>::iterator end() {
		return data.end();
	}
};


template <class ElementType, class ElementAllocator,
	class ContainerAllocatorSpecial, class Comparator = std::less<ElementType>>
class SortTestGenerator {
private:
	const unsigned long long lcgA = 211, lcgC = 25731, lcgM = 1000000000000037;
	mutable unsigned long long lcgX;

	unsigned long long nextRandom() const {
		return lcgX = (lcgX * lcgA + lcgC) % lcgM;
	}

	const ElementAllocator& elementCreator;
	const Comparator comparator;

public:

	SortTestGenerator(unsigned long long seed, ElementAllocator& elementCreator,
			const Comparator& comparator = Comparator())
		:lcgX(seed), elementCreator(elementCreator), comparator(comparator)
	{}

	const SortTest<ElementType, ContainerAllocatorSpecial, Comparator> nextRandomTest(unsigned int size) const {
		ElementType* els = new ElementType[size];
		for (unsigned int i = 0; i < size; ++i) {
			els[i] = elementCreator(nextRandom());
		}

		SortTest<ElementType, ContainerAllocatorSpecial, Comparator> test(els, size, comparator);
		delete[] els;
		return test;
	}

	const SortTest<ElementType, ContainerAllocatorSpecial, Comparator>
			nextRunSequenceTest(unsigned int runSize, unsigned int runsCount) const {
		unsigned int size = runSize * runsCount;

		ElementType* els = new ElementType[size];
		for (unsigned int i = 0; i < size; ++i) {
			els[i] = elementCreator(nextRandom());
		}

		for (unsigned int i = 0; i < size; i += runSize) {
			std::sort(els + i, els + (i + runSize), comparator);
		}

		SortTest<ElementType, ContainerAllocatorSpecial, Comparator> test(els, size, comparator);
		delete[] els;
		return test;
	}
};





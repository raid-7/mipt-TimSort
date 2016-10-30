#include <algorithm>
#include <iostream>
#include <cmath>
#include <string>
#include <sstream>

#include "sort-test.h"
#include "timsort.h"


int intAllocator(unsigned long long random) {
	return random & 0xFFFFFFFF;
}

std::string stringAllocator(unsigned long long random) {
	std::basic_ostringstream<char> out;
	for (unsigned int i = 0; i < random % 7200; ++i) {
		out << (random ^ (((i * 17) << 16) + i));
	}
	return out.str();
}

std::string* stringPointerAllocator(unsigned long long random) {
	return new std::string(stringAllocator(random));
}

double doubleAllocator(unsigned long long random) {
	return static_cast<double>(random) * std::pow(0.9, static_cast<double>(random & 0xF));
}


class Point {
private:
	double x, y, z;

public:
	Point()
		:Point(0.0, 0.0, 0.0)
	{}

	Point(double x, double y, double z)
		:x(x), y(y), z(z)
	{}

	double getX() const {
		return x;
	}
	double getY() const {
		return y;
	}
	double getZ() const {
		return z;
	}
};

Point pointAllocator(unsigned long long random) {
	unsigned long long
		rnd1 = random & 0xFFFF,
		rnd2 = (random >> 16) & 0xFFFF,
		rnd3 = (random >> 32) & 0xFFFF;

	return Point(doubleAllocator(rnd1), doubleAllocator(rnd2), doubleAllocator(rnd3));
}

// Sort with distance to pivot
class PointComparator {
private:
	Point pivot;

public:
	PointComparator(const Point& pivot)
		:pivot(pivot)
	{}

	bool operator ()(const Point& a, const Point& b) const {
		double da = std::pow(pivot.getX() - a.getX(), 2.0) +
						std::pow(pivot.getY() - a.getY(), 2.0) +
						std::pow(pivot.getZ() - a.getZ(), 2.0);
		double db = std::pow(pivot.getX() - b.getX(), 2.0) +
						std::pow(pivot.getY() - b.getY(), 2.0) +
						std::pow(pivot.getZ() - b.getZ(), 2.0);
		return da < db;
	}
};

class StringPointerComparator {
public:
	bool operator ()(std::string* const a, std::string* const b) const {
		return (*a) < (*b);
	}
};

class TimParams1: public ITimSortParams {
public:
	unsigned int minRun(unsigned int n) const {
		int res = n & 0x1F;
		while (n) {
			n = n & (n - 1);
			++res;
		}
		return res;
	}

	bool needMerge(unsigned int lenX, unsigned int lenY) const {
		return lenX > lenY;
	}

	EWhatMerge whatMerge(unsigned int lenX, unsigned int lenY, unsigned int lenZ) const {
		if (lenX <= lenY && lenX + lenY <= lenZ)
			return WM_NoMerge;

		if (lenX < lenZ)
			return WM_MergeXY;

		return WM_MergeYZ;
	}

	unsigned int GetGallop() const {
		return 32;
	}
};
class TimParams2: public ITimSortParams {
public:
	unsigned int minRun(unsigned int n) const {
		int res = n & 0xF;
		while (n) {
			n = n & (n - 1);
			res += 2;
		}
		return res;
	}

	bool needMerge(unsigned int lenX, unsigned int lenY) const {
		return lenX + 2 > lenY;
	}

	EWhatMerge whatMerge(unsigned int lenX, unsigned int lenY, unsigned int lenZ) const {
		if (lenX <= lenY + 4 && lenX + lenY <= lenZ + 8)
			return WM_NoMerge;

		if (lenX < lenZ)
			return WM_MergeXY;

		return WM_MergeYZ;
	}

	unsigned int GetGallop() const {
		return 1;
	}
};
class TimParamsBad: public ITimSortParams {
public:
	unsigned int minRun(unsigned int n) const {
		int res = n & 0x1F;
		while (n) {
			n = n & (n - 1);
			++res;
		}
		return res;
	}

	bool needMerge(unsigned int lenX, unsigned int lenY) const {
		return lenX < lenY;
	}

	EWhatMerge whatMerge(unsigned int lenX, unsigned int lenY, unsigned int lenZ) const {
		if (lenX > lenY && lenX + lenY > lenZ)
			return WM_NoMerge;

		if (lenX > lenZ)
			return WM_MergeXY;

		return WM_MergeYZ;
	}

	unsigned int GetGallop() const {
		return 1;
	}
};


class SortingFunctor {
private:
	const bool stdSort;

public:
	SortingFunctor(bool std)
		:stdSort(std)
	{}

	template <class RandomAccessIterator, class Compare>
	void operator ()(RandomAccessIterator first, RandomAccessIterator last,
			const Compare& comp, const ITimSortParams& params) const {
		if (stdSort)
			std::sort(first, last, comp);
		else
			TimSort(first, last, comp, params);
	}
	template <class RandomAccessIterator, class Compare>
	void operator ()(RandomAccessIterator first, RandomAccessIterator last, const Compare& comp) const {
		if (stdSort)
			std::sort(first, last, comp);
		else
			TimSort(first, last, comp);
	}
	template <class RandomAccessIterator>
	void operator ()(RandomAccessIterator first, RandomAccessIterator last, const ITimSortParams& params) const {
		if (stdSort)
			std::sort(first, last);
		else
			TimSort(first, last, params);
	}
	template <class RandomAccessIterator>
	void operator ()(RandomAccessIterator first, RandomAccessIterator last) const {
		if (stdSort)
			std::sort(first, last);
		else
			TimSort(first, last);
	}
};

template <class ElementType, class ContainerAllocatorSpecial, class Comparator = std::less<ElementType>>
void runComparingTest(SortTest<ElementType, ContainerAllocatorSpecial, Comparator> test, std::string comment) {
	std::cout << comment << "\n";
	SortTestResult timResult = test.applyTest(SortingFunctor(false));
	SortTestResult stdResult = test.applyTest(SortingFunctor(true));
	std::cout << " TimSort:\n  " << timResult.toString() << '\n';
	std::cout << " StdSort:\n  " << stdResult.toString() << '\n';
	std::cout << '\n';
}
template <class ElementType, class ContainerAllocatorSpecial, class Comparator = std::less<ElementType>>
void runComparingTest(SortTest<ElementType, ContainerAllocatorSpecial, Comparator> test,
			const ITimSortParams& params, std::string comment) {
	std::cout << comment << "\n";
	SortTestResult timResult = test.applyTest(SortingFunctor(false), &params);
	SortTestResult stdResult = test.applyTest(SortingFunctor(true));
	std::cout << " TimSort:\n  " << timResult.toString() << '\n';
	std::cout << " StdSort:\n  " << stdResult.toString() << '\n';
	std::cout << '\n';
}


void testSimpleCases() {
	SortTestGenerator<int, int (unsigned long long), VectorAllocator<int>> intVectorGenerator(717, intAllocator);

	runComparingTest(intVectorGenerator.nextRandomTest(50), "50 ints in vector");
	runComparingTest(intVectorGenerator.nextRandomTest(4000), "4000 ints in vector");
	runComparingTest(intVectorGenerator.nextRandomTest(8000000), "8000000 ints in vector");

	SortTestGenerator<int, int (unsigned long long), ArrayAllocator<int>> intArrayGenerator(717, intAllocator);

	runComparingTest(intArrayGenerator.nextRandomTest(50), "50 ints in array");
	runComparingTest(intArrayGenerator.nextRandomTest(4000), "4000 ints in array");
	runComparingTest(intArrayGenerator.nextRandomTest(8000000), "8000000 ints in array");

	runComparingTest(intArrayGenerator.nextRandomTest(0), "An empty array test");
	runComparingTest(intVectorGenerator.nextRandomTest(0), "An empty vector test");
}

void testTimParams() {
	TimParams1 params1;
	TimParams2 params2;
	TimParamsBad paramsBad;

	SortTestGenerator<double, double (unsigned long long),
			ArrayAllocator<double>> doubleVectorGenerator(3112907, doubleAllocator);

	SortTest<double, ArrayAllocator<double>> smallTest =
			doubleVectorGenerator.nextRandomTest(25000);
	SortTest<double, ArrayAllocator<double>> mediumTest =
			doubleVectorGenerator.nextRandomTest(125000);
	SortTest<double, ArrayAllocator<double>> largeTest =
			doubleVectorGenerator.nextRandomTest(1000000);

	runComparingTest(smallTest, params1, "25000 doubles in vector, Params 1");
	runComparingTest(smallTest, params2, "25000 doubles in vector, Params 2");
	runComparingTest(smallTest, paramsBad, "25000 doubles in vector, Params bad");
	runComparingTest(smallTest, "25000 doubles in vector, Params default");

	runComparingTest(mediumTest, params1, "125000 doubles in vector, Params 1");
	runComparingTest(mediumTest, params2, "125000 doubles in vector, Params 2");
	runComparingTest(mediumTest, "125000 doubles in vector, Params default");

	runComparingTest(largeTest, params1, "1000000 doubles in vector, Params 1");
	runComparingTest(largeTest, params2, "1000000 doubles in vector, Params 2");
	runComparingTest(largeTest, "1000000 doubles in vector, Params default");
}


void testParitalSortedOne(const SortTestGenerator<int, int (unsigned long long),
			ArrayAllocator<int>>& gen, unsigned int runSize, unsigned int runsCount) {
	std::basic_ostringstream<char> oStr;
	oStr << runsCount << " runs of int with length " << runSize << " in array";
	runComparingTest(gen.nextRunSequenceTest(runSize, runsCount), oStr.str());
}
void testPartialSorted() {
	SortTestGenerator<int, int (unsigned long long), ArrayAllocator<int>> intArrayGenerator(29, intAllocator);

	unsigned int runSizes[] {20, 40, 80, 128, 1024};
	unsigned int runCounts[] {2, 4, 10, 100, 1000, 10000};

	for (unsigned int i = 0; i < 6; ++i) {
		for (unsigned int j = 0; j < 5; ++j) {
			testParitalSortedOne(intArrayGenerator, runSizes[j], runCounts[i]);
		}
	}
}

void testStrings() {
	SortTestGenerator<std::string, std::string (unsigned long long), ArrayAllocator<std::string>>
				stringArrayGenerator(2514, stringAllocator);

	runComparingTest(stringArrayGenerator.nextRandomTest(1000), "1000 strings in array");
	runComparingTest(stringArrayGenerator.nextRandomTest(4000), "4000 strings in array");
	runComparingTest(stringArrayGenerator.nextRandomTest(12000), "12000 strings in array");

	SortTestGenerator<std::string*, std::string* (unsigned long long),
			ArrayAllocator<std::string*>, StringPointerComparator>
				stringPointerArrayGenerator(2514, stringPointerAllocator, StringPointerComparator());

	runComparingTest(stringPointerArrayGenerator.nextRandomTest(1000), "1000 string pointers in array");
	runComparingTest(stringPointerArrayGenerator.nextRandomTest(4000), "4000 string pointers in array");
	runComparingTest(stringPointerArrayGenerator.nextRandomTest(12000), "12000 string pointers in array");
}

void testEtalones() {
	SortTestGenerator<int, int (unsigned long long), VectorAllocator<int>> intVectorGenerator(717, intAllocator);
	SortTestGenerator<int, int (unsigned long long), ArrayAllocator<int>> intArrayGenerator(717, intAllocator);

	runComparingTest(intVectorGenerator.nextRandomTest(10000000), "10000000 random ints in vector");
	runComparingTest(intArrayGenerator.nextRandomTest(10000000), "10000000 random ints in array");
}

void testPoints() {
	PointComparator comparator(Point(7.35e3, 1.194e2, 6.832e-2));
	SortTestGenerator<Point, Point (unsigned long long), ArrayAllocator<Point>, PointComparator>
				pointArrayGenerator(72514, pointAllocator, comparator);

	runComparingTest(pointArrayGenerator.nextRandomTest(1000000), "1000000 random 3d-points in array");
	runComparingTest(pointArrayGenerator.nextRunSequenceTest(1000, 1000),
				"1000 runs of 3d-points with length 1000 in array");
}

int main() {
	testEtalones();
	testSimpleCases();
	testPartialSorted();
	testTimParams();
	testStrings();
	testPoints();
}

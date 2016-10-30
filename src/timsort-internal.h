#include <stack>
#include <cmath>
#include <iterator>
#include <typeinfo>



class DefaultTimSortParams: public ITimSortParams {
public:
	unsigned int minRun(unsigned int n) const {
		return 48;
	}

	bool needMerge(unsigned int lenX, unsigned int lenY) const {
		return lenX >= lenY;
	}

	EWhatMerge whatMerge(unsigned int lenX, unsigned int lenY, unsigned int lenZ) const {
		if (lenX < lenY && lenX + lenY < lenZ)
			return WM_NoMerge;

		if (lenX < lenZ)
			return WM_MergeXY;

		return WM_MergeYZ;
	}

	unsigned int GetGallop() const {
		return 7;
	}
};


template <class SortIterator,
	class Comparator = std::less<typename std::iterator_traits<SortIterator>::value_type>>
class TimSortController {
private:
	typedef typename std::iterator_traits<SortIterator>::value_type Value;
	typedef typename std::iterator_traits<SortIterator>::difference_type Distance;

	class RunController;

	const SortIterator begin, end;
	const Comparator& comparator;
	const ITimSortParams& params;
	std::stack<RunController> runStack;

	TimSortController(const SortIterator& begin, const SortIterator& end,
			const Comparator& comparator, const ITimSortParams& params)
		:begin(begin), end(end), comparator(comparator), params(params) {}


	void sort() {
		unsigned int minRunSize = params.minRun(static_cast<unsigned int>(end - begin));

		SortIterator lastIndexIterator = begin;

		while (lastIndexIterator < end) {
			unsigned int curMinSize =
					std::min(minRunSize, static_cast<unsigned int>(end - lastIndexIterator));
			RunController nextRun =
					RunController::makeRun(lastIndexIterator, lastIndexIterator+curMinSize, end, *this);
			lastIndexIterator = nextRun.end();
			pushRun(nextRun);
			checkStack();
		}

		while (runStack.size() > 1) {
			RunController x = popRun();
			RunController y = popRun();
			mergeRuns(y, x);
			pushRun(y);
			checkStack();
		}
	}

	void checkStack() {
		if (runStack.size() == 2) {
			RunController x = popRun();
			RunController y = popRun();

			bool merge = params.needMerge(x.size(), y.size());
			if (merge) {
				mergeRuns(y, x);
				pushRun(y);
			} else {
				pushRun(y);
				pushRun(x);
			}
		} else if (runStack.size() > 2) {
			RunController x = popRun();
			RunController y = popRun();
			RunController z = popRun();

			EWhatMerge whatMerge = params.whatMerge(x.size(), y.size(), z.size());
			if (whatMerge == WM_NoMerge) {
				pushRun(z);
				pushRun(y);
				pushRun(x);
			} else if (whatMerge == WM_MergeXY) {
				mergeRuns(y, x);
				pushRun(z);
				pushRun(y);

				checkStack();
			} else if (whatMerge == WM_MergeYZ) {
				mergeRuns(z, y);
				pushRun(z);
				pushRun(x);

				checkStack();
			}
		}

//		std::stack<RunController> tmp;
//		while (!runStack.empty()) {
//			RunController rc = popRun();
//			std::cout << rc.size() << ' ';
//			tmp.push(rc);
//		}
//		while (!tmp.empty()) {
//			pushRun(tmp.top());
//			tmp.pop();
//		}
//		std::cout << '\n';
	}

	void mergeRuns(RunController& x, RunController& y) const {
		inplaceMerge(x.begin(), y.begin(), y.end());
		x.join(y);
	}

	void inplaceMerge(SortIterator b, SortIterator m, SortIterator e) const {

		unsigned int fullSize = static_cast<unsigned int>(e - b);
		unsigned int blockSize = static_cast<unsigned int>(std::sqrt(fullSize));
		unsigned int blocksCount = (fullSize + blockSize - 1) / blockSize;
		unsigned int yellowId = -1;

		if (blocksCount < 5) {
			// Insert sort
			RunController::makeRun(b, e, e, *this);
			return;
		}

//		RunController::getUnsortedRunPointer(b, m, *this)->coutRunContent();
//		RunController::getUnsortedRunPointer(m, e, *this)->coutRunContent();

		// make decomposition
		RunController** blocks = new RunController*[blocksCount];
		for (unsigned int i = 0; i < blocksCount; ++i) {
			blocks[i] = RunController::getUnsortedRunPointer(
						b + blockSize * i, std::min(e, b + blockSize * (i + 1)), *this);
			if (blocks[i]->begin() <= m && blocks[i]->end() > m) {
				yellowId = i;
			}
		}
		RunController::swapRuns(*(blocks[yellowId]), *(blocks[blocksCount - 2]));
		yellowId = blocksCount - 2;

		// selection sort of blocks
		for (unsigned int i = 0; i < yellowId; ++i) {
			unsigned int minRun = i;
			SortIterator minIt = blocks[minRun]->begin();

			for (unsigned int j = i + 1; j < yellowId; ++j) {
				SortIterator it = blocks[j]->begin();
				if (comparator(*it, *minIt)) {
					minIt = it;
					minRun = j;
				}
			}

			if (minRun != i)
				RunController::swapRuns(*blocks[i], *blocks[minRun]);
		}

		// merge neighbour blocks without join
		for (unsigned int i = 0; i < yellowId-1; ++i) {
			RunController* x = blocks[i];
			RunController* y = blocks[i + 1];
			simpleMerge(x->begin(), x->end(), y->begin(), y->end(), blocks[yellowId]->begin());
		}

		unsigned int s = blocks[blocksCount - 1]->size() + blocks[yellowId]->size();
		RunController::makeRun(e - s * 2, e, e, *this);


		// iterative merge
		SortIterator buf = e - s;
		SortIterator gammaIterator = buf;
		SortIterator betaIterator = gammaIterator - s;
		SortIterator alphaIterator = betaIterator - s;
		while (betaIterator > b) {
			if (alphaIterator < b)
				alphaIterator = b;

			simpleMerge(alphaIterator, betaIterator, betaIterator, gammaIterator, buf);

			gammaIterator = betaIterator;
			betaIterator = alphaIterator;
			alphaIterator -= s;
		}

		// sort buffer (tail with length s)
		RunController::makeRun(buf, e, e, *this);

		for (unsigned int i = 0; i < blocksCount; ++i)
			delete blocks[i];
		delete[] blocks;
	}

	void simpleMerge(const SortIterator& b1, const SortIterator& e1,
				const SortIterator& b2, const SortIterator& e2, const SortIterator& buffer) const {

		if ((e1 - b1) > (e2 - b2)) {
			simpleMerge(b2, e2, b1, e1, buffer);
			return;
		}

		SortIterator itMain1 = b1;
		SortIterator itBuf = buffer;
		while (itMain1 < e1) {
			swapIterators(itMain1++, itBuf++);
		}

		itMain1 = buffer;
		SortIterator itMain2 = b2;
		SortIterator itRes = b1;

		bool lastComparison = false;
		int sameComparisonCount = -1;
		unsigned int gallop = params.GetGallop();
		while (itMain1 < itBuf || itMain2 < e2) {
			if (itMain1 == itBuf) {
				swapIterators(itRes++, itMain2++);
			} else if (itMain2 == e2) {
				swapIterators(itRes++, itMain1++);
			} else {
				bool comparison = comparator(*itMain1, *itMain2);
				if (comparison == lastComparison && sameComparisonCount != -1) {
					sameComparisonCount++;
					if (sameComparisonCount == static_cast<int>(gallop)) {
						unsigned int needCopies = comparison ?
									findCopiesCount(itMain1, itBuf, itMain2, true) :
									findCopiesCount(itMain2, e2, itMain1, true);
//						std::cout << needCopies << ' ' << (comparison ? itBuf - itMain1 : e2 - itMain2) << '\n';
						while (needCopies && --needCopies) {
							swapIterators(itRes++, comparison ? itMain1++ : itMain2++);
						}
					}
				} else {
					lastComparison = comparison;
					sameComparisonCount = 0;
				}
				swapIterators(itRes++, comparison ? itMain1++ : itMain2++);
			}
			if (itRes == e1) {
				itRes = b2;
			}
		}
	}

	unsigned int findCopiesCount(const SortIterator& b, const SortIterator& e,
				const SortIterator& pivot, bool expectedComparison) const {
		unsigned int l = 0, r = 1;
		while ((b + r) < e && comparator(b[r], *pivot) == expectedComparison) {
			r <<= 1;
			if (e - b <= r) {
				r = e - b;
				break;
			}
		}

		while (l < r) {
			unsigned int m = (l + r) >> 1;
			if (comparator(b[m], *pivot) == expectedComparison) {
				l = m + 1;
			} else {
				r = m;
			}
		}

		return l > 0 ? l - 1 : 0;
	}

	RunController popRun() {
		RunController rc = runStack.top();
		runStack.pop();
		return rc;
	}
	void pushRun(RunController rc) {
		runStack.push(rc);
	}

	class RunController {
	private:
		SortIterator _begin, _end;

	public:
		const TimSortController<SortIterator, Comparator>& parentController;

		void join(const RunController& run) {
			_end = run._end;
		}

		unsigned int size() const {
			return static_cast<unsigned int>(_end - _begin);
		}

		const SortIterator begin() const {
			return _begin;
		}
		const SortIterator end() const {
			return _end;
		}

		void coutRun() const {
			std::cout << "Run[" << _begin - parentController.begin <<
						", " << _end - parentController.begin << ")=" << size() << ";\n";
		}
		void coutRunContent() const {
			coutRun();
			for (SortIterator it = _begin; it < _end; ++it) {
				std::cout << ' ' << *it;
			}
			std::cout << '\n';
		}

	private:
		RunController(SortIterator begin, SortIterator end,
				const TimSortController<SortIterator, Comparator>& parentController)
			:_begin(begin), _end(end), parentController(parentController)
		{}

		void sortRun() const {
			for (SortIterator it = _begin + 1; it < _end; ++it) {
				SortIterator t = it;
				Value v = *it;
//				std::cout << typeid(v).name() << "]]]\n";
				while (t > _begin && !parentController.comparator(t[-1], v)) {
					*t = *(t-1);
					--t;
				}
				*t = v;
			}
		}
		void reverseRun() const {
			SortIterator a = _begin;
			SortIterator b = _end;

			while (a < b) {
				swapIterators(a++, --b);
			}
		}

	public:
		static void swapRuns(const RunController& a, const RunController& b) {
			SortIterator it1 = a._begin;
			SortIterator it2 = b._begin;

			while (it1 < a._end && it2 < b._end) {
				swapIterators(it1++, it2++);
			}
		}
		static RunController makeRun(SortIterator start, SortIterator minPos, SortIterator finish,
					const TimSortController<SortIterator, Comparator>& tsController) {
			SortIterator begin = start;
			SortIterator end = start + 1;
			bool compareType = false;
			bool resortFlag = false;

			if (end != finish) {

				compareType = tsController.comparator(*end, *start);
				++end;

				while (end < finish && tsController.comparator(end[0], end[-1]) == compareType) {
					++end;
				}
				while (end < minPos && end < finish) {
					++end;
					resortFlag = true;
				}

			}

			RunController controller(begin, end, tsController);
			if (resortFlag)
				controller.sortRun();
			else if (compareType)
				controller.reverseRun();

			return controller;
		}
		static RunController* getUnsortedRunPointer(SortIterator start, SortIterator finish,
					const TimSortController<SortIterator, Comparator>& tsController) {
			return new RunController(start, finish, tsController);
		}
	};

	template <class T>
	static void swap(T& a, T& b) {
		T t = a;
		a = b;
		b = t;
	}
	static void swapIterators(SortIterator a, SortIterator b) {
		swap(*a, *b);
	}


public:
	static void sort(SortIterator begin, SortIterator end,
			const Comparator& comparator, const ITimSortParams& params = DefaultTimSortParams()) {

		if (begin == end)
			return;

		TimSortController controller(begin, end, comparator, params);
		controller.sort();
	}

	static void sort(SortIterator begin, SortIterator end,
			const ITimSortParams& params = DefaultTimSortParams()) {
		sort(begin, end, Comparator(), params);
	}
};

#ifndef KMERPROCESSOR_H_
#define KMERPROCESSOR_H_

#include <KmerCounter.h>

namespace kmers
{

class KmerProcessor
{
public:
	KmerProcessor(const char* begin, size_t inputSize, int k, int n, int numOfThreads) : _begin(begin), _inputSize(inputSize), _processingDone(false), _k(k), _n(n), _numOfThreads(numOfThreads)
	{
		HashTableConfig hc(1000, 15);
		for(int i=0;i<numOfThreads;i++)
		{
			_counters.push_back(KmerCounter(begin, begin+inputSize, k, n, hc));
		}
	}

	// TODO maybe we want this to be async?
	void process(vector<pair<string, size_t>>& results)
	{
		for(int i=0;i<_counters.size();i++)
			_threads.push_back(std::thread(&KmerCounter::process, std::ref(_counters[i])));

		for(int i=0;i<_threads.size();i++)
			_threads[i].join();

		_processingDone = true;

		_collectMostCommons(results);

	}


protected:

	void _collectMostCommons(vector<pair<string, size_t>>& results) const
	{
		vector<pair<string, size_t>> all;
		for(const KmerCounter& km : _counters)
		{
			vector<pair<string, size_t>> tmp;
			km.getTopStrings(tmp);
			std::copy(tmp.begin(),tmp.end(), back_inserter(all));
		}
		std::sort(all.begin(), all.end(), [](const pair<string, size_t>& lhs, const pair<string, size_t>& rhs)
											{
												if(lhs.second >= rhs.second)
													return true;
												else
													return false;
											});
		// TODO refactor - extract method below
		int count=0;
		size_t prevSize = 0;
		for(int i =0;i < all.size();i++)
		{
			const auto& tmp = all[i];
			if(prevSize!=tmp.second)
					count++;
			prevSize = tmp.second;
			if(count>_n)
				break;

			results.push_back(tmp);

		}
	}

protected:
	const char*			_begin;
	size_t				_inputSize;
	int					_k;
	int					_n;
	int					_numOfThreads;
	bool				_processingDone;
	vector<KmerCounter> _counters;
	vector<std::thread> _threads;
};

}

#endif
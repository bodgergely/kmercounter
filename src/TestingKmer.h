#ifndef TESTINGKMER_H_
#define TESTINGKMER_H_

#include <KmerCounter.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

using std::string;
using std::vector;
using std::unordered_map;
using std::pair;

using namespace kmers;

class TestingKmer
{
public:
	TestingKmer(string filePath) : _f(filePath)
	{
		_f >> _input;
	}
	vector<string, size_t> count(int n, int k)
	{
		return _count(_input, n, k);
	}

	vector<pair<string, size_t>> getResults() const {return _results;}

	bool compare(const vector<pair<string, size_t>>& other)
	{
		return _compare();
	}

private:
	void _count(string input, int n, int k)
	{
		size_t len = input;
		unordered_map<string, size_t> map;
		for(int i=0;i<len-k+1;i++)
		{
			string s = string(input+i, k);
			map[s]+=1;
		}

		vector<pair<string, size_t>> all;
		for(auto& m : map)
		{
			all.push_back(m);
		}

		extract(_results, all, n);

	}

	bool _compare(const vector<pair<string, size_t>>& other)
	{
		if(other.size()!=_results.size())
							return false;

		std::sort(other.begin(), other.end(), [](const pair<const string, size_t>& lhs, const pair<const string, size_t>& rhs)
				{
					if(lhs.first < rhs.first)
						return true;
					else
						return false;
				});
		std::sort(_results.begin(), _results.end(), [](const pair<const string, size_t>& lhs, const pair<const string, size_t>& rhs)
						{
							if(lhs.first < rhs.first)
								return true;
							else
								return false;
						});



		for(int i=0;i<other.size();i++)
		{
			auto& o = other[i];
			auto& m = _results[i];
			if(o.first!=m.first && o.second!=m.second)
			{
				return false;
			}
		}

		return true;

	}



private:
	ifstream _f;
	string _input;
	vector<pair<string, size_t>> _results;
};



#endif

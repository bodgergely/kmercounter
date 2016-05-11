/*
 * count.cpp
 *
 *  Created on: May 3, 2016
 *      Author: geri
 */

#include <KmerEngine.h>
#include <Mer.h>

#ifdef _TESTING
#include <TestingKmer.h>
#endif

#include <iostream>
#include <vector>
#include <utility>
#include <string>

using namespace kmers;
using namespace std;

int main(int argc, char** argv)
{
	string file = string(argv[1]);
	int n = atoi(argv[2]);
	int k = atoi(argv[3]);
	int threadCount = 4;

	KmerEngine engine(file, k, n, threadCount);
	engine.start();
	vector<pair<string, size_t>> results = engine.getResults();
	unsigned long long totalkmerCount = engine.totalKmerCount();


	for(const auto& p : results)
	{
		cout << p.first << "," << p.second << endl;
	}
	cout << "Finished!\n";

#ifdef _TESTING
	TestingKmer tester(file);
	tester.count(n, k);
	bool pass = tester.compare(results);

	vector<pair<string, size_t>> testresults = tester.getResults();
	cout << "test results:\n";
	for(const auto& p : results)
	{
		cout << p.first << "," << p.second << endl;
	}
	cout << "\n";

	if(pass)
		cout << "Test passed!\n";
	else
		cout << "Test failed!\n";
#endif


	return 0;
}





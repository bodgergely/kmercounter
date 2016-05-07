/*
 * count.cpp
 *
 *  Created on: May 3, 2016
 *      Author: geri
 */

#include <KmerEngine.h>

#include <iostream>
#include <vector>
#include <utility>
#include <string>

using kmers::KmerEngine;
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

	for(const auto& p : results)
	{
		cout << p.first << "," << p.second << endl;
	}

	cout << "Finished!\n";


	return 0;
}





/*
 * count.cpp
 *
 *  Created on: May 3, 2016
 *      Author: geri
 */

#include <kmers.h>

#include <iostream>
#include <vector>
#include <utility>
#include <string>

using kmers::KmerProcessor;
using namespace std;

int main(int argc, char** argv)
{
	KmerProcessor processor("abcbdefdfdf", 3, 10, 1);

	vector<pair<string, size_t>> results;
	processor.process(results);

	for(const auto& p : results)
	{
		cout << p.first << "," << p.second << endl;
	}

	return 0;
}





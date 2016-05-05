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
	const char* str = "abcbdebbfdfdfebb";
	int k = 3;
	int n = 10;

	if(argc > 1)
	{
		str = argv[1];
		k = atoi(argv[2]);
		n = atoi(argv[3]);
	}


	size_t len = strlen(str);
	char* input = (char*)malloc((len+1) * sizeof(char));
	strcpy(input, str);
	KmerProcessor processor(input,len,k, n, 1);

	vector<pair<string, size_t>> results;
	processor.process(results);

	for(const auto& p : results)
	{
		cout << p.first << "," << p.second << endl;
	}

	return 0;
}





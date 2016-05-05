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
	size_t len = strlen(str);
	char* input = (char*)malloc((len+1) * sizeof(char));
	strcpy(input, str);
	printf("%s\n", input);
	KmerProcessor processor(input,len,3, 15, 1);

	vector<pair<string, size_t>> results;
	processor.process(results);

	for(const auto& p : results)
	{
		cout << p.first << "," << p.second << endl;
	}

	return 0;
}





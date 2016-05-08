#ifndef QUINTTREE_H_
#define QUINTTREE_H_

#include <Defines.h>

#include <string>
#include <stdexcept>
#include <cassert>
#include <memory>
#include <vector>
#include <queue>
#include <list>
#include <utility>
#include <algorithm>
#include <unordered_set>

using std::string;
using std::unique_ptr;
using std::vector;
using std::priority_queue;
using std::pair;
using std::make_pair;
using std::list;
using std::unordered_set;

class Node
{
public:
	Node(char val, Node* parent, Node* a, Node* c, Node* g, Node* t, Node* n, int count=0) : _val(val),
																		_parent(parent), _a(a),
																		_c(c), _g(g), _t(t), _n(n), _count(count)
	{
	}
	char   value() {return _val;}
	Node*  parent() {return _parent;}
	Node** a() {return &_a;}
	Node** c() {return &_c;}
	Node** g() {return &_g;}
	Node** t() {return &_t;}
	Node** n() {return &_n;}
	int	  count() {return _count;}
	void  count(int count) {_count = count;}

private:
	char  _val;
	Node* _parent;
	Node* _a;
	Node* _c;
	Node* _g;
	Node* _t;
	Node* _n;
	int		_count;	// only used at the leaf
};

bool NodeCountCompare(const pair<Node*, unsigned long>& lhs,const pair<Node*, unsigned long>& rhs)
{
	if(lhs.second < rhs.second)
		return true;
	else
		return false;
}


class QuintTree
{
public:
	/*
	 * n is the topmost elem size
	 * k is kmer len
	 */
	QuintTree(int n, int k) : root(0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr), _n(n), _k(k), _count(0)
	{

	}

	void insert(const char* begin, const char* end, unsigned int count)
	{
			Node* currnode = &root;
			for(const char* s=begin; s!=end; s++)
			{
				char c = *s;
				currnode = next(currnode, c);
			}
			currnode->count(currnode->count()+count);
			unsigned long currCount = currnode->count();


			// TODO factor this out to a method
			if(currCount >= _minCount)
			{
				_bestNodes.push_back(make_pair(currnode, currCount));
				// increase count of counts if not yet in list
				if(currCount>_minCount && _bestSizes.find(currCount) == _bestSizes.end())
					_count++;
				_bestSizes.insert(currCount);
				if(_minCount==0)
					_minCount = currCount;
				if(_count > _n)
				{
					// pop the min count elements and also determine the next min elem
					int nextMin = std::numeric_limits<int>::max();
					for(auto it = _bestNodes.begin();it!=_bestNodes.end();)
					{
						if(it->second == _minCount)
						{
							it = _bestNodes.erase(it);
						}
						else
						{
							if(it->second < nextMin){
								nextMin = it->second;
							}
							it++;
						}
					}
					// invariance restored
					_minCount = nextMin;
					_count--;
				}
			}
			else
			{
				if(_count < _n)
				{
					_bestNodes.push_back(make_pair(currnode, currCount));
					_count++;
					_minCount = currCount;
					_bestSizes.insert(currCount);
				}
			}


			assert(_count<=_n);

	}

	void insert(const string& kmer, unsigned int count)
	{
		insert(kmer.c_str(), kmer.c_str()+kmer.size(), count);
	}

	vector<pair<string, unsigned long>> getResults()
	{
		for(auto it=_bestNodes.begin();it!=_bestNodes.end();it++)
		{
			Node* node = it->first;
			string s = rebuild(node);
			_result.push_back(make_pair(s, it->second));
		}

		std::sort(_result.begin(),_result.end(),[](const pair<string, unsigned long>& lhs, const pair<string, unsigned long>& rhs)
												{
													if(lhs.second > rhs.second)
														return true;
													else
														return false;
												});

		return _result;
	}

	size_t numberOfNodes() const {return _nodeAllocCount;}

private:

	string rebuild(Node* leaf) const
	{
		vector<char> kmer(_k, 0);

		Node* node = leaf;

		// while we are not at the root
		int i = _k-1;
		while(node->parent()!=nullptr)
		{
			kmer[i] = node->value();
		    --i;
		    node = node->parent();
		}

		string s;
		std::copy(kmer.begin(), kmer.end(), back_inserter(s));

		return s;
	}

	Node* next(Node* parent, char c)
	{
		Node** nextNode = nullptr;
		//if(c=='a')
		//	printf("Parent is: %p\n", parent);
		switch(c)
		{
		case 'a':
			nextNode = parent->a();
			//printf("next node is at: %p and has value: %p\n", nextNode, *nextNode);
			break;
		case 'c':
			nextNode = parent->c();
			break;
		case 'g':
			nextNode = parent->g();
			break;
		case 't':
			nextNode = parent->t();
			break;
		case 'n':
			nextNode = parent->n();
			break;
		default:
			throw std::runtime_error("Invalid character: " + c);

		}

		if(*nextNode == nullptr)
		{
			++_nodeAllocCount;
			*nextNode = new Node(c, parent, NULL, NULL, NULL, NULL, NULL);
			//if(c == 'a')
			//	printf("next node is at: %p and has value: %p and parent->a()is at: %p and has value: %p and parent is: %p\n", nextNode, *nextNode, parent->a(), *parent->a(), parent);
		}

		//printf("Node size: %d and object: %d\n", sizeof(Node), sizeof(**nextNode));

		//if(c=='a')
		//	assert(parent->a()!=NULL);

		return *nextNode;

	}


private:
	size_t _n;
	size_t _k;
	size_t _nodeAllocCount = 0;
	Node root;
	unsigned long _count;		// how many we have so far from the top most elems
	unsigned long _minCount = 0;
	list<pair<Node*, unsigned long>> _bestNodes;
	unordered_set<unsigned long>     _bestSizes;
	vector<pair<string, unsigned long>> _result;

};

#endif

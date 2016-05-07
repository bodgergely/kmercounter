#ifndef STOPWATCH_H_
#define STOPWATCH_H_

#include <chrono>

using namespace std::chrono;

template<class TimeRes>
class StopWatch
{
public:
	void start()
	{
		_start = high_resolution_clock::now();
	}
	size_t stop()
	{
		_end = high_resolution_clock::now();
		return duration_cast<TimeRes>(_end-_start).count();
	}
private:
	time_point<high_resolution_clock> _start;
	time_point<high_resolution_clock> _end;
};


#endif

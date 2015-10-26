
#ifndef USERDB_LOCKFREE_QUEUE_H_
#define USERDB_LOCKFREE_QUEUE_H_

#include <queue>
#include <thread>
#include <mutex>

template<typename T>
class lockfree_queue
{
	using value_t = T;
public:
	void push(const value_t & val)
	{
		std::lock_guard<std::mutex> grd(mtx);
		data.push(val);
	}

	bool empty() const
	{
		std::lock_guard<std::mutex> grd(mtx);
		return data.empty();
	}

	value_t take()
	{
		std::lock_guard<std::mutex> grd(mtx);
		
		value_t res = data.front();
		data.pop();
		return std::move(res);
	}

	size_t size() const
	{
		std::lock_guard<std::mutex> grd(mtx);
		return data.size();
	}

private:
	std::queue<value_t> data;
	mutable std::mutex mtx;
};

#endif

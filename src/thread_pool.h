
#ifndef USERDB_THREAD_POOL_H_
#define USERDB_THREAD_POOL_H_

#include <thread>
#include <atomic>
#include <condition_variable>
#include <vector>
#include <queue>

struct task_base
{
	virtual ~task_base()
	{}

	virtual void execute() = 0;
};

template<typename F>
struct task_wrapper : public task_base
{
	task_wrapper(F f)
		:fun(f)
	{
	}

	virtual void execute() override
	{
		fun();
	}

	F fun;
};


class thread_pool
{
public:
	thread_pool()
		:workers(std::thread::hardware_concurrency(), nullptr), finished(false), busyCount(0), totalTasksExecuted(0)
	{
		for(unsigned i = 0; i < workers.size(); ++i)
			workers[i] = new std::thread(&thread_pool::worker_thread, this);

		cout << "Thread pool started (" << workers.size() << " threads)" << std::endl;
	}

	~thread_pool()
	{
		for(auto w : workers)
			delete w;
	}


	template<typename F>
	void submit(F task)
	{
		{
			std::lock_guard<std::mutex> grd(qmtx);
			tasks.push(new task_wrapper<F>(task));	
		}

		std::unique_lock<std::mutex> grd(wmtx);
		cv.notify_one();
	}

	void wait_finished()
	{
		{
			std::unique_lock<std::mutex> grd(wmtx);
			cv.notify_all();
		}

		while(worker_body());
		finished.store(true);

		{
			std::unique_lock<std::mutex> grd(wmtx);
			cv.notify_all();
		}

		join_all();
	}

	unsigned total_executed_tasks() const
	{
		return totalTasksExecuted.load();
	}

	bool is_busy() const
	{
		return (busyCount.load() > 0);
	}

	unsigned worker_count() const
	{
		return workers.size();
	}
private:
	void worker_thread()
	{
		while(!finished)
		{
			if(!worker_body())
			{
				std::unique_lock<std::mutex> grd(wmtx);
				cv.wait(grd);
			}			
		}
	}

	//Returns true, if there was a task
	bool worker_body()
	{
		task_base * p_task = nullptr;
		{
			std::lock_guard<std::mutex> grd(qmtx);
			if(!tasks.empty())
			{
				p_task = tasks.front();
				tasks.pop();
			}
		}

		if(p_task)
		{
			++busyCount;
			p_task->execute();
			++totalTasksExecuted;
			--busyCount;
		}

		delete p_task;

		return (p_task != nullptr);
	}

	void join_all()
	{
		for(auto w : workers)
			w->join();
	}

private:
	std::vector<std::thread*> workers;
	std::atomic<bool> finished;
	std::queue<task_base*> tasks;

	std::mutex qmtx, wmtx;
	std::condition_variable cv;
	std::atomic<unsigned> busyCount;
	std::atomic<unsigned> totalTasksExecuted;
};

#endif



#ifndef ULTRA_CRYPT_OPENMP_ENGINE
#define ULTRA_CRYPT_OPENMP_ENGINE

#include "engine.h"
#include "lockfree_queue.h"
#include "mpi.h"
#include <chrono>
#include <mutex>
#include <thread>
#include <set>

using namespace std;
using namespace std::chrono;

inline void log_print()
{
	std::cout << std::endl;
}

template <typename Type, typename ... ArgsTypes>
inline void log_print(const Type & value, const ArgsTypes& ... args)
{
	std::cout << value;
	log_print(args ...);
}

std::mutex log_mtx;
template <typename ... ArgsTypes>
inline void safe_log_print(const ArgsTypes& ... args)
{
	std::unique_lock<std::mutex> grd(log_mtx);
	log_print(args ...);
}

#define LOG(...) {}
//#define LOG(...) safe_log_print(__VA_ARGS__)

template<typename D>
std::ostream & to_human_dura(std::ostream& os, D dura)
{
    typedef duration<int, ratio<86400>> days;
    char fill = os.fill();
    os.fill('0');
    auto d = duration_cast<days>(dura);
    dura -= d;
    auto h = duration_cast<hours>(dura);
    dura -= h;
    auto m = duration_cast<minutes>(dura);
    dura -= m;
    auto s = duration_cast<seconds>(dura);

    if(d.count() > 0)
    	os << d.count() << " days ";

    if(h.count() > 0)
    	os << h.count() << " hours ";
    if(m.count() > 0)
    	os << m.count() << " min ";
    
    os << s.count() << " sec";
    os.fill(fill);

    return os;
};

struct task_res_t
{
	task_res_t()
		:lastMsg(false)
	{}

	friend std::ostream & operator<<(std::ostream & os, const task_res_t & tr)
	{
		return os << "#" << tr.chunk_id << ", found=" << tr.found << ", last=" << tr.lastMsg;
	}

	bool found, lastMsg;
	typename key_manager::key_id_t foundKeyId;
	unsigned chunk_id;
};

//http://bisqwit.iki.fi/story/howto/openmp/
template<typename B>
class openmp_engine : public bf_engine_base<B>
{
	
public:
	using bf_t = B;
	using _Base = bf_engine_base<B>;

	openmp_engine(bf_t & p_bf)
		:_Base(&p_bf), queueSize(5)
	{	
	}

	struct controller
	{
		struct pending_chunk_t
		{
			pending_chunk_t(unsigned tid)
				:completed(false), thread_id(tid)
			{}

			unsigned thread_id, keyCount;
			bool completed;
			typename key_manager::key_id_t firstKey;
		};

		chunk_generator<bf_t> gen;
		std::map<unsigned, pending_chunk_t> pendingChunks;
		openmp_engine * pEngine;

		controller(openmp_engine * eng, typename bf_t::uint_t start_key)
			:pEngine(eng), gen(*eng->pBF, start_key, eng->chunkSize)
		{
		}

		void send_chunk(unsigned receiver_id, const bf_task & cur_chunk)
		{
			LOG("Sending chunk ", cur_chunk, " to ", receiver_id);
			MPI::COMM_WORLD.Send(&cur_chunk, sizeof(cur_chunk), MPI::CHAR, receiver_id, 0);

			if(cur_chunk.id > 0)
			{
				pending_chunk_t pen_chunk(receiver_id);
				pen_chunk.firstKey = cur_chunk.first;
				pen_chunk.keyCount = cur_chunk.last - cur_chunk.first;
				pendingChunks.insert(make_pair(cur_chunk.id, pen_chunk));
			}
		}
		
		void run(unsigned worker_count)
		{
			cout << "Starting control thread..." << std::endl;

			auto first_key = gen.cur_idx;

			MPI::Status status;

			auto start_tp = std::chrono::system_clock::now();

			cout << "Sending initial queued chunks..." << std::endl;

			for(unsigned i = 0; i < worker_count; ++i)
				for(unsigned q = 0; q < pEngine->queueSize; ++q)
					send_chunk(i + 1, gen.next_chunk());

			cout << "Starting main loop..." << std::endl;

			while(true)
			{
				task_res_t res_task;

				MPI::COMM_WORLD.Recv(&res_task, sizeof(res_task), MPI::CHAR, MPI_ANY_SOURCE, 0, status);
				int sender_id = status.Get_source();

				LOG("Recieved completed task (sender=", sender_id, ", res=", res_task.found, ", chunk_id=", res_task.chunk_id, ")");

				if(res_task.chunk_id > 0)
				{
					auto it = pendingChunks.find(res_task.chunk_id);
					if(it == pendingChunks.end())
						throw std::runtime_error("Pending chunks corrupted");
					it->second.completed = true;

					typename key_manager::key_id_t last_processed_key;

					LOG("first pending chunk is #", pendingChunks.begin()->first);
					bool something_removed = false;
					auto rit = pendingChunks.begin();
					while((rit != pendingChunks.end()) && (rit->second.completed))
					{
						last_processed_key = rit->second.firstKey + rit->second.keyCount;
						//cout << "Deleting from pending #" << rit->first << std::endl;
						rit = pendingChunks.erase(rit);
						something_removed = true;
					}

					if(something_removed)
					{
						auto now_tp = std::chrono::system_clock::now();
						auto first_chunk = *pendingChunks.begin();

						unsigned cur_key_len = pEngine->pBF->key_length(last_processed_key);
						auto keys_left = pEngine->pBF->total_keys_including_length(cur_key_len) - last_processed_key;

						auto last_key = last_processed_key;
						auto keys_done = last_key - first_key;
						std::chrono::duration<double> diff = now_tp - start_tp;

						double keys_per_sec = keys_done / diff.count();
						double secs_left = keys_left / keys_per_sec;
						std::chrono::seconds dura_left(static_cast<uint64_t>(secs_left));
						
						cout << "Last key #" << big_uint(last_key) << " (" << pEngine->pBF->get_ekey(last_key) << "), speed " << keys_per_sec << " keys/sec, left ";
						to_human_dura(cout, dura_left) << " till finishing keys with length " << cur_key_len << std::endl;
					}
				}
				else
					throw std::runtime_error("Recieved 0-id chunk");

				if(res_task.found)	//Terminate
				{
					pEngine->correctKey = pEngine->pBF->get_ekey(res_task.foundKeyId);
					//this->correctKey.resize(res_task.key_length);
					//MPI::COMM_WORLD.Recv((char*)this->correctKey.data(), res_task.key_length, MPI::CHAR, sender_id, 0);

					cout << "Key found (by process #" << sender_id << "), terminating..." << std::endl;

					std::set<unsigned> unfinished_workers;

					for(unsigned i = 0; i < worker_count; ++i)
					{
						send_chunk(i + 1, bf_task());
						if(i + 1 != sender_id)
							unfinished_workers.insert(i + 1);
					}

					while(!unfinished_workers.empty())
					{
						LOG("waiting ", unfinished_workers.size(), " for termination...");
						MPI::COMM_WORLD.Recv(&res_task, sizeof(res_task), MPI::CHAR, MPI_ANY_SOURCE, 0, status);
						if(res_task.lastMsg)
							unfinished_workers.erase(status.Get_source());
					}
					/*--worker_count;

					
					while(worker_count > 0)
					{
						MPI::COMM_WORLD.Recv(&res_task, sizeof(res_task), MPI::CHAR, MPI_ANY_SOURCE, 0, status);
						sender_id = status.Get_source();
						MPI::COMM_WORLD.Send(&empty_chunk, sizeof(empty_chunk), MPI::CHAR, sender_id, 0);
						--worker_count;

						//cout << "Sent empty chunk to " << sender_id << std::endl;
					}*/

					break;
				}
				else
					send_chunk(sender_id, gen.next_chunk());
			}
		}
	};

	struct searcher
	{
		searcher(openmp_engine * eng, unsigned _id)
			:pEngine(eng), processId(_id)
		{}

		void run()
		{
			cout << "Starting search process #" << processId << "..." << std::endl;

			bf_task cur_chunk;

			std::thread rcv_thread(&searcher::receiver_thread, this);
			std::thread snd_thread(&searcher::sender_thread, this);

			task_res_t task_res;
			task_res.found = false;
			task_res.chunk_id = std::numeric_limits<unsigned>::max();

			while((!task_res.found) && (task_res.chunk_id > 0))
			{
				{
					std::unique_lock<std::mutex> grd(inData.mtx);
					if(inData.queue.empty())
					{
						LOG("Thread #", processId, " waiting for new data...");
						inData.cv.wait(grd);
					}
				}

				cur_chunk = inData.queue.take();

				LOG("searcher #", processId, " processing chunk ", cur_chunk);

				if(cur_chunk.id > 0)
					task_res.found = (*pEngine->pBF)(cur_chunk.key_length, cur_chunk.first, cur_chunk.last, task_res.foundKeyId);

				if(task_res.found)
					LOG("searcher  #", processId, " found correct key");
				
				task_res.chunk_id = cur_chunk.id;
				LOG("searcher  #", processId, " commiting res: ", task_res);

				outData.queue.push(task_res);

				std::unique_lock<std::mutex> grd(outData.mtx);
                outData.cv.notify_all();
			}

			rcv_thread.join();
			snd_thread.join();
			cout << "Finished search process #" << processId << "." << std::endl;
		}

		void receiver_thread()
		{
			bf_task new_chunk;
			do
			{
				MPI::COMM_WORLD.Recv(&new_chunk, sizeof(new_chunk), MPI::CHAR, 0, 0);
				LOG("receiver_thread #", processId, " got chunk ", new_chunk);

				inData.queue.push(new_chunk);

				std::unique_lock<std::mutex> grd(inData.mtx);
                inData.cv.notify_all();
			} while(new_chunk.id > 0);

			LOG("receiver_thread #", processId, " finished");
		}

		void sender_thread()
		{
			task_res_t new_res;
			new_res.found = false;
			while((!new_res.found) && (!new_res.lastMsg))
			{
				{
					std::unique_lock<std::mutex> grd(outData.mtx);
					if(outData.queue.empty())
					{
						LOG("sender_thread #", processId, " waiting for data... ");
						outData.cv.wait(grd);
					}
				}

				new_res = outData.queue.take();
				if(new_res.chunk_id == 0)
					new_res.lastMsg = true;

				LOG("sender_thread #", processId, " sending ", new_res);
				MPI::COMM_WORLD.Send(&new_res, sizeof(new_res), MPI::CHAR, 0, 0);
			}

			LOG("sender_thread #", processId, " finished");

			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		openmp_engine * pEngine;
		unsigned processId;

		//std::mutex mtxIn, mtxOut;
		struct
		{
			lockfree_queue<bf_task> queue;
			std::mutex mtx;
			std::condition_variable cv;
		} inData;

		struct
		{
			lockfree_queue<task_res_t> queue;
			std::mutex mtx;
			std::condition_variable cv;
		} outData;
		
	};

	virtual bool search(typename bf_t::uint_t start_key) override
	{
		int rank, size;//, len;
	    //char version[MPI_MAX_LIBRARY_VERSION_STRING];
		MPI::Init();
	    rank = MPI::COMM_WORLD.Get_rank();
	    size = MPI::COMM_WORLD.Get_size();
	    //MPI_Get_library_version(version, &len);

	    if(rank == 0)
	    {
	    	controller ctrl(this, start_key);
	    	ctrl.run(size - 1);
	    }
	    else
	    {
	    	searcher srch(this, rank);
	    	srch.run();
	    }
		MPI::Finalize();

		return (rank == 0);
	}

	unsigned chunkSize;
private:
	const unsigned queueSize;
};

#endif

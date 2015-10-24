
#ifndef ULTRA_CRYPT_OPENMP_ENGINE
#define ULTRA_CRYPT_OPENMP_ENGINE

#include "engine.h"
//#include <omp.h>
#include "mpi.h"


//http://bisqwit.iki.fi/story/howto/openmp/

template<typename B>
class openmp_engine : public bf_engine_base<B>
{
	struct task_res_t
	{
		bool found;
		unsigned key_length;
	};
public:
	using bf_t = B;
	using _Base = bf_engine_base<B>;

	openmp_engine(bf_t & p_bf)
		:_Base(&p_bf)
	{	
	}

	virtual bool search() override
	{
		int rank, size;//, len;
	    //char version[MPI_MAX_LIBRARY_VERSION_STRING];
		MPI::Init();
	    rank = MPI::COMM_WORLD.Get_rank();
	    size = MPI::COMM_WORLD.Get_size();
	    //MPI_Get_library_version(version, &len);
	    //std::cout << "Hello, world!  I am " << rank << " of " << size << "(" << version << ", " << len << ")" << std::endl;

	    if(rank == 0)
	    	control_thread(size - 1);
	    else
	    	search_thread(rank);
		MPI::Finalize();

		return (rank == 0);
	}

	void search_thread(unsigned thread_id)
	{
		cout << "Starting search thread #" << thread_id << "..." << std::endl;

		task_res_t task_res;
		task_res.found = false;

		MPI::COMM_WORLD.Send(&task_res, sizeof(task_res), MPI::CHAR, 0, 0);

		typename bf_t::key_t found_key;

		while(!task_res.found)
		{
			bf_task cur_chunk;
			MPI::COMM_WORLD.Recv(&cur_chunk, sizeof(cur_chunk), MPI::CHAR, 0, 0);
			cout << "Thread #" << thread_id << " got chunk " << cur_chunk << std::endl;
			if(cur_chunk.id == 0)
				break;
			task_res.found = (*this->pBF)(cur_chunk.key_length, cur_chunk.first, cur_chunk.last, found_key);
			task_res.key_length = found_key.size();
			MPI::COMM_WORLD.Send(&task_res, sizeof(task_res), MPI::CHAR, 0, 0);

			if(task_res.found)
				MPI::COMM_WORLD.Send(found_key.data(), found_key.size(), MPI::CHAR, 0, 0);
		}

		cout << "Finished search thread # " << thread_id << "." << std::endl;
	}

	void control_thread(unsigned worker_count)
	{
		cout << "Starting control thread..." << std::endl;

		chunk_generator<bf_t> gen(*this->pBF);

		MPI::Status status;

		while(true)
		{
			task_res_t res_task;

			MPI::COMM_WORLD.Recv(&res_task, sizeof(res_task), MPI::CHAR, MPI_ANY_SOURCE, 0, status);
			int sender_id = status.Get_source();

			cout << "Recieved completed task (sender=" << sender_id << ", res="  << res_task.found << ")" << std::endl;

			if(res_task.found)	//Terminate
			{
				this->correctKey.resize(res_task.key_length);
				MPI::COMM_WORLD.Recv((char*)this->correctKey.data(), res_task.key_length, MPI::CHAR, sender_id, 0);

				cout << "Key found, terminating..." << std::endl;
				--worker_count;

				bf_task empty_chunk;
				while(worker_count > 0)
				{
					MPI::COMM_WORLD.Recv(&res_task, sizeof(res_task), MPI::CHAR, MPI_ANY_SOURCE, 0, status);
					sender_id = status.Get_source();
					MPI::COMM_WORLD.Send(&empty_chunk, sizeof(empty_chunk), MPI::CHAR, sender_id, 0);
					--worker_count;

					cout << "Sent empty chunk to " << sender_id << std::endl;
				}

				break;
			}
			else
			{
				auto cur_chunk = gen.next_chunk();
				cout << "Sending chunk " << cur_chunk << " to " << sender_id << std::endl;
				MPI::COMM_WORLD.Send(&cur_chunk, sizeof(cur_chunk), MPI::CHAR, sender_id, 0);
			}
		}
	}
};

#endif

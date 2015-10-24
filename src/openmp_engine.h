
#ifndef ULTRA_CRYPT_OPENMP_ENGINE
#define ULTRA_CRYPT_OPENMP_ENGINE

#include "engine.h"
//#include <omp.h>
#include "mpi.h"


//http://bisqwit.iki.fi/story/howto/openmp/

template<typename B>
class openmp_engine : public bf_engine_base<B>
{
public:
	using bf_t = B;
	using _Base = bf_engine_base<B>;

	openmp_engine(bf_t & p_bf)
		:_Base(&p_bf)
	{	
	}

	virtual void search() override
	{
		chunk_generator<bf_t> gen(*this->pBF);

		std::atomic<bool> found(false);
		{
			//cout << "Starting OPENMP thread " << omp_get_thread_num() << " (total " << omp_get_num_threads() << " threads)" << std::endl;
			int rank, size;//, len;
		    //char version[MPI_MAX_LIBRARY_VERSION_STRING];
			MPI::Init();
		    rank = MPI::COMM_WORLD.Get_rank();
		    size = MPI::COMM_WORLD.Get_size();
		    //MPI_Get_library_version(version, &len);
		    //std::cout << "Hello, world!  I am " << rank << " of " << size << "(" << version << ", " << len << ")" << std::endl;
		    
		    cout << "Starting search thread #" << rank << '/' << size << std::endl;
			do
			{
				typename bf_t::key_t found_key;

				auto cur_chunk = gen.next_chunk();
				bool r = (*this->pBF)(cur_chunk.key_length, cur_chunk.first, cur_chunk.last, found_key);
				cout << "Thread " << rank << ": processed " << cur_chunk << " (res=" << r << ")" << std::endl;
				if(r)
				{
					found.store(true);
					this->correctKey = found_key;
				}
			} while(!found.load());

			MPI::Finalize();
		}
	}
};

#endif

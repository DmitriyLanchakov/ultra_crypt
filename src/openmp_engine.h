
#ifndef ULTRA_CRYPT_OPENMP_ENGINE
#define ULTRA_CRYPT_OPENMP_ENGINE

#include "engine.h"
#include <omp.h>

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
#pragma omp parallel
		{
			cout << "Starting OPENMP thread " << omp_get_thread_num() << " (total " << omp_get_num_threads() << " threads)" << std::endl;
			do
			{
				typename bf_t::key_t found_key;

				auto cur_chunk = gen.next_chunk();
				bool r = (*this->pBF)(cur_chunk.key_length, cur_chunk.first, cur_chunk.last, found_key);
				cout << "Thread " << omp_get_thread_num() << ": processed " << cur_chunk << " (res=" << r << ")" << std::endl;
				if(r)
				{
					found.store(true);
					this->correctKey = found_key;
				}
			} while(!found.load());
		}
	}
};

#endif

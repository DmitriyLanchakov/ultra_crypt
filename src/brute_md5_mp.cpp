

#include "brute_forcer.h"
#include "md5.h"
#include "engine.h"
#include <omp.h>
#include <atomic>

struct md5_crypter
{
	using value_t = std::string;
	
	value_t operator()(const std::string & code) const
	{
		return md5(code);
	}
};

//md5 of 'grape': b781cbb29054db12f88f08c6e161c199

using bf_t = brute_forcer<md5_crypter>;

int main(int argc, char ** argv)
{
	omp_set_num_threads(4);

	std::atomic<unsigned> val(0);

#pragma omp parallel

	for(unsigned i = 0; i < 10; ++i)
	{
		cout << "thread " << omp_get_thread_num() << '/' << omp_get_num_threads() << ":" << val++ << std::endl;
	}
	

	return 0;
}


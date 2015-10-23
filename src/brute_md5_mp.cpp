

#include "brute_forcer.h"
#include "md5.h"
#include "engine.h"
#include <omp.h>

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

#pragma omp parallel
	printf("Hello from thread %d, nthreads %d\n", omp_get_thread_num(), omp_get_num_threads());

	return 0;
}


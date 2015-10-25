

#include "brute_forcer.h"
#include "md5.h"
#include "openmp_engine.h"
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
	//omp_set_num_threads(4);

	std::atomic<unsigned> val(0);

	std::string alphabet = "abcdefghijklmnopqrstuvwxyz0123456789_"; //"b781cbb29054db12f88f08c6e161c199"
	bf_t bforcer(md5_crypter(), md5("grape_p12"), alphabet);

	openmp_engine<bf_t> eng(bforcer);
	eng();

	return 0;
}


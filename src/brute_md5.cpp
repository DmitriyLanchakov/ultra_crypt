

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
	std::string alphabet = "abcdefghijklmnopqrstuvwxyz0123456789_";
	bf_t bforcer(md5_crypter(), "b781cbb29054db12f88f08c6e161c199", alphabet);
	linear_brute_force(bforcer);
	return 0;
}


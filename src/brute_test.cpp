
#include "engine.h"
#include <functional>
#include <iostream>
#include <thread>
#include <chrono>
#include <queue>


using namespace std;

int main(int argc, char ** argv)
{
	std::string correct_code = "hi_allj";

	struct crypter
	{
		using value_t = size_t;
		
		size_t operator()(const std::string & code) const
		{
			return hash_fn(code);
		}

		std::hash<std::string> hash_fn;
	};

	crypter cpt;

	std::string alphabet = "abcdefghijklmnopqrstuvwxyz0123456789_";
	//std::string alphabet = "ahil_";

	using bf_t = brute_forcer<crypter>;
	bf_t bforcer(cpt(correct_code), alphabet);

	/*for(unsigned i = 0; i < 1000000U; ++i)
	{
		auto ikey = bforcer.to_alpha_radix(i);
		unsigned r = bforcer.to_number(ikey);

		if(i != r)
			cout << i << '=' << r << std::endl;
	}*/

	auto correct_id = bforcer.to_number(correct_code);
	cout << "Correct word id=" << correct_id << std::endl;

	
	//linear_brute_force(bforcer);
	//thread_pool_brute_force(bforcer);
	multithread_engine<bf_t> eng(bforcer);
	eng();

	

	return 0;
}


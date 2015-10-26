
#include "tester.h"
#include "brute_forcer.h"
#include "big_int.h"
#include <random>

using namespace std;

/*
std::ostream & operator<<(std::ostream & os, __uint128_t val)
{
	return os << "$$";
}*/

SURVEY_TEST(big_int)
{
	for(unsigned i = 0; i < 1000; ++i)
	{
		//cout << i << "=" << big_uint(i) << std::endl;
	}
	cout << big_uint(1) << std::endl;

	SURVEY_TEST_EQ(big_uint("1"), big_uint(1));
	SURVEY_TEST_EQ(big_uint("10"), big_uint(16));
	SURVEY_TEST_EQ(big_uint("ff"), big_uint(255));
	SURVEY_TEST_EQ(big_uint("16B78EDCF7"), big_uint(97568873719));

	cout << "=================" << std::endl;


	cout << big_uint("100") << std::endl;


	for(unsigned i = 0; i < 10000; ++i)
	{
		std::stringstream stream;
		stream << std::hex << i;
		std::string hex_uint(stream.str());

		//cout << i << "=>" << hex_uint << std::endl;
		SURVEY_TEST_EQ(to_string(big_uint(hex_uint)), hex_uint);
	}
	
	//cout << big_uint("0a85918ae8838727c5551") << std::endl;

}

SURVEY_TEST(key_manager_simple)
{
	const std::string alphabet = "abc";
	key_manager mgr(alphabet);

	SURVEY_TEST_EQ(mgr.key_length(0), 1);
	SURVEY_TEST_EQ(mgr.key_length(2), 1);
	SURVEY_TEST_EQ(mgr.key_length(3), 2);
	SURVEY_TEST_EQ(mgr.key_length(11), 2);
	SURVEY_TEST_EQ(mgr.key_length(12), 3);


	SURVEY_TEST_EQ((unsigned)mgr.keys_of_length(0), 0);
	SURVEY_TEST_EQ((unsigned)mgr.keys_of_length(1), 3);
	SURVEY_TEST_EQ((unsigned)mgr.keys_of_length(2), 9);
	SURVEY_TEST_EQ((unsigned)mgr.keys_of_length(3), 27);

	SURVEY_TEST_EQ((unsigned)mgr.total_keys_including_length(0), 0);
	SURVEY_TEST_EQ((unsigned)mgr.total_keys_including_length(1), 3);
	SURVEY_TEST_EQ((unsigned)mgr.total_keys_including_length(2), 12);

	//numeric_key_t({0, 0});

	//for(unsigned i = 0; i < 15; ++i)
	//	cout << i << ": " << mgr.get_ikey(i) << " => "<< mgr.get_ekey(i) << std::endl;

	SURVEY_TEST_EQ(mgr.get_ekey(0), "a");
	SURVEY_TEST_EQ(mgr.get_ekey(1), "b");
	SURVEY_TEST_EQ(mgr.get_ekey(3), "aa");
	SURVEY_TEST_EQ(mgr.get_ekey(12), "aaa");

	SURVEY_TEST_EQ((unsigned)mgr.get_key_id("cc"), 11);
	SURVEY_TEST_EQ((unsigned)mgr.get_key_id("aaa"), 12);
	SURVEY_TEST_EQ((unsigned)mgr.get_key_id("aab"), 13);

}

SURVEY_TEST(key_manager)
{
	std::cout << "testing..." << std::endl;

	const std::string alphabet = "abcdefghijklmnopqrstuvwxyz0123456789_'";
	key_manager mgr(alphabet);

	SURVEY_TEST_EQ((unsigned)mgr.keys_of_length(0), 0);
	SURVEY_TEST_EQ((unsigned)mgr.keys_of_length(1), alphabet.size());
	SURVEY_TEST_EQ((unsigned)mgr.keys_of_length(3), alphabet.size() * alphabet.size() * alphabet.size());

	//Keys count
	SURVEY_TEST_EQ(ui_power<unsigned>(2, 4), 16);
	SURVEY_TEST_EQ(ui_power<unsigned>(2, 8), 256);
	SURVEY_TEST_EQ(ui_power<unsigned>(2, 16), 65536);

	SURVEY_TEST_EQ((unsigned)mgr.total_keys_including_length(0), 0);
	SURVEY_TEST_EQ((unsigned)mgr.total_keys_including_length(1), alphabet.size());

	typename key_manager::key_id_t key_count = alphabet.size();
	for(unsigned i = 2; i < 20; ++i)
	{
		key_count += ui_power<typename key_manager::key_id_t>(alphabet.size(), i);
		
		auto cc = mgr.total_keys_including_length(i);

		if(cc == 0)
			throw std::runtime_error("Overflow");
		SURVEY_TEST_EQ(big_uint(cc), big_uint(key_count));

		//cout << "len=" << i << ", key_count=" << big_uint(key_count) << ", cc=" << big_uint(cc) << std::endl;

		SURVEY_TEST_EQ(mgr.key_length(cc - 1), i);
		SURVEY_TEST_EQ(mgr.key_length(cc), i + 1);
		SURVEY_TEST_EQ(mgr.key_length(cc + 1), i + 1);
	}

	//Key lengths
	SURVEY_TEST_EQ(mgr.get_ikey(0).size(), 1);

	for(unsigned i = 0; i < alphabet.size(); ++i)
	{
		SURVEY_TEST_EQ(mgr.get_ikey(i).size(), 1);
	}

	SURVEY_TEST_EQ(mgr.get_ikey(alphabet.size() + 1).size(), 2);

	cout << mgr.get_ikey(0) << std::endl;
	cout << mgr.get_ikey(1) << std::endl;
	cout << mgr.get_ikey(2) << std::endl;
	//Key restore
	for(unsigned i = 1; i < alphabet.size(); ++i)
	{
		SURVEY_TEST_EQ(mgr.get_ekey(i), std::string(1, alphabet[i]));
	}

	default_random_engine generator;
    uniform_int_distribution<uint64_t> distribution(0, std::numeric_limits<uint64_t>::max());

    for(unsigned i = 0; i < 1000; ++i)
    {
    	typename key_manager::key_id_t random_id = distribution(generator);
    	auto skey = mgr.get_ekey(random_id);

    	auto obtained_id = mgr.get_key_id(skey);
    	SURVEY_TEST_EQ(big_uint(random_id), big_uint(obtained_id));
    }

    {
    	std::string src_key = "yuraho4etkushat'";
    	auto kid = mgr.get_key_id(src_key);
    	cout << "kid=" << big_uint(kid) << std::endl;
    	auto ckey = mgr.get_ekey(kid);
    	cout << ckey << std::endl;
    	SURVEY_TEST_EQ(src_key, ckey);

    	std::string str_id = "a850918ae8838727c5551";
    	SURVEY_TEST_EQ(to_string(big_uint(kid)), str_id);
    	auto ekey = mgr.get_ekey(big_uint(str_id).data);
    	SURVEY_TEST_EQ(src_key, ekey);
    }

    //auto
    //cout << big_uint() << std::endl;
    //cout << big_uint("00000a85918ae8838727c5551").data);

    

	/*for(unsigned i = 0; i < 200; ++i)
	{
		cout << "#" << i << "=>" << mgr.get_ikey(i) << std::endl;
	}*/
}



int main(int argc, char ** argv)
{
	tester_singleton::singleton()->exec();

	return 0;
}

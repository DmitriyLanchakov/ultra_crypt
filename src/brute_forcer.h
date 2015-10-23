
#ifndef ULTRA_CRYPT_BRUTE_FORCER
#define ULTRA_CRYPT_BRUTE_FORCER

#include <cstdint>
#include <string>
#include <math.h>
#include <vector>
#include <iostream>

using namespace std;

template<typename C>
class brute_forcer
{
public:
	using checker_t = C;
	using value_t = typename checker_t::value_t;
	using uint_t = uint64_t;
	using key_t = std::string;
	using internal_key_t = std::vector<uint8_t>;

	brute_forcer(value_t val, const key_t & _alphabet)
		:correctVal(val), alphabet(_alphabet), radix(_alphabet.size())
	{}

	bool operator()(unsigned key_length, uint_t first, uint_t last, key_t & correctKey) const
	{
		internal_key_t cur_key = to_alpha_radix(first);
		cur_key.resize(key_length, 0);

		key_t str_key(key_length, 0);

		while(first != last)
		{
			to_alpha_key(cur_key, str_key);

			//std::cout << "Checking key: " << str_key << std::endl;
			if(checker(str_key) == correctVal)
			{
				correctKey = str_key;
				return true;
			}

			next_key(cur_key);
			++first;
		}

		return false;
	}

	uint_t keys_of_length(unsigned key_length) const
	{
		uint_t res = 1;
		while(key_length-- > 0)
			res *= radix;
		return res;
	}

	key_t get_ith_key(uint_t index) const
	{
		internal_key_t ikey = to_alpha_radix(index);
		
		key_t res(ikey.size(), 0);
		to_alpha_key(ikey, res);
		
		return std::move(res);
	}

	internal_key_t to_alpha_radix(uint_t index) const
	{
		internal_key_t res(static_cast<unsigned>(ceil(log(index) / log(radix))) + 1, 0);

		unsigned r = 0, i = 0;

		while(index > 0)
		{
			r = index % radix;
			index = (index - r) / radix;
			res[i++] = r;
		}

		return std::move(res);
	}

	void next_key(internal_key_t & key) const
	{
		++key[0];
		unsigned i = 0;

		for(; (i < key.size() - 1) && (key[i] == radix); ++i)
		{
			key[i] = 0;
			++key[i + 1];
		}

		/*if(*key.rbegin() == radix)
		{
			*key.rbegin() = 0;
			key.push_back(0);
		}*/
	}

	uint_t to_number(const internal_key_t & key) const
	{
		uint_t res = 0;
		uint_t cur_mult = 1;
		for(unsigned i = 0; i < key.size(); ++i)
		{
			res += cur_mult * key[i];
			cur_mult *= radix;
		}

		return res;
	}

	uint_t to_number(const key_t & alpha_key) const
	{
		internal_key_t ikey;
		convert(alpha_key, ikey);

		cout << alpha_key << "=>";
		print(cout, ikey) << std::endl;

		return to_number(ikey);
	}

	std::ostream & print(std::ostream & os, const internal_key_t & key) const
	{
		for(auto digit : key)
			os << (unsigned)digit << ':';
		return os;
	}
private:
	void convert(const key_t & alpha_key, internal_key_t & int_key) const
	{
		int_key.resize(alpha_key.size());
		for(unsigned i = 0; i < alpha_key.size(); ++i)
		{
			//int_key[i] = std::distance(alphabet.begin(), std::find(alphabet.begin(), alphabet.end(), alpha_key[i]));
			int_key[i] = alphabet.find(alpha_key[i]);
		}
	}

	void to_alpha_key(const internal_key_t & int_key, key_t & alpha_key) const
	{
		unsigned s = int_key.size();
		for(unsigned i = 0; i < s; ++i)
			alpha_key[s - i - 1] = alphabet[int_key[i]];
	}	

private:
	checker_t checker;
	value_t correctVal;
	const key_t alphabet;
	const unsigned radix;
};

#endif

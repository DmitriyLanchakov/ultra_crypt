
#ifndef ULTRA_CRYPT_BRUTE_FORCER
#define ULTRA_CRYPT_BRUTE_FORCER

#include "big_int.h"
#include <cstdint>
#include <string>
#include <math.h>
#include <vector>
#include <iostream>
#include <mutex>

using namespace std;

struct numeric_key_t
{
	numeric_key_t(unsigned len)
		:data(len, 0x00)
	{}

	/*numeric_key_t(std::initizlizer_list vals)
		:data(vals)
	{
	}*/


	uint8_t & operator[](unsigned idx)
	{
		return data[idx];
	}

	uint8_t operator[](unsigned idx) const
	{
		return data[idx];
	}

	unsigned size() const
	{
		return data.size();
	}

	friend std::ostream & operator<<(std::ostream & os, const numeric_key_t & key)
	{
		for(auto digit : key.data)
			os << (unsigned)digit << ':';
		return os;
	}

	std::vector<uint8_t> data;
};

template<typename T>
T ui_power(T base, T pow)
{
	T res = 1;
	while(pow-- > 0)
		res *= base;
	return res;
}

class key_manager
{
public:
	using key_t = std::string;
	using internal_key_t = numeric_key_t;
	//using key_id_t = uint64_t;
	using key_id_t = __uint128_t;

	key_manager(const key_t & _alphabet)
		:alphabet(_alphabet), radix(alphabet.size())
	{

	}

	key_id_t keys_of_length(unsigned key_length) const
	{
		if(key_length > 0)
		{
			key_id_t res = 1;
			while(key_length-- > 0)
				res *= radix;
			return res;
		}
		else
			return 0;
	}

	key_t get_ekey(key_id_t index) const
	{
		internal_key_t ikey = get_ikey(index);
		
		key_t res(ikey.size(), 0);
		to_alpha_key(ikey, res);
		
		return std::move(res);
	}

	void to_alpha_key(const internal_key_t & int_key, key_t & alpha_key) const
	{
		unsigned s = int_key.size();
		for(unsigned i = 0; i < s; ++i)
			alpha_key[s -1 - i] = alphabet[int_key[i]];
	}

	internal_key_t get_ikey(key_id_t key_id) const
	{
		internal_key_t res(key_length(key_id));

		key_id_t local_id = key_id - total_keys_including_length(res.size() - 1);

		//cout << "key_len=" << key_length(key_id) << std::endl;

		key_id_t r = 0;
		unsigned i = 0, s = res.size();

		while(local_id > 0)
		{
			//cout << "r=" << big_uint(r) << ", key_id=" << big_uint(key_id) << std::endl;
			r = local_id % radix;
			local_id = (local_id - r) / radix;
			res[i++] = r;
		}

		//cout << "=>";
		//print(cout, res) << std::endl;

		return std::move(res);
	}

	unsigned key_length(key_id_t key_id) const
	{

		/*
		{
			double approx_len = log(double(key_id) * (radix - 1) / radix + 1) / log(double(radix));
			unsigned len = static_cast<unsigned>(ceil(approx_len));

			cout << "key_id=" << big_uint(key_id) << ", approx_len=" << approx_len << ", len=" << len << ", total_k(len-1)=" << big_uint(total_keys_including_length(len - 1)) << ", total_k(len)=" << big_uint(total_keys_including_length(len)) << std::endl;

			if()
				return len;
			else
				return len - 1;
		}
		*/

		if(key_id == 0)
			return 1;
		else
		{
			unsigned res = 0;
			key_id_t prev_count = 0, cur_count = 0;

			do
			{
				++res;
				//cout << "invalid: res=" << res << ", prev=" << big_uint(prev_count) << ", id=" << big_uint(key_id) << ", cur=" << big_uint(cur_count) << std::endl;
				prev_count = cur_count;
				cur_count = total_keys_including_length(res);
			}
			while(!((prev_count <= key_id) && (key_id < cur_count)));

			//
			return res;
		}
	}

	key_id_t total_keys_including_length(unsigned length) const
	{
		if(length > 0)
		{
			key_id_t r = radix;
			return r * (ui_power<key_id_t>(r, length) - 1) / (r - 1);
		}
		else
			return 0;
	}

	key_id_t get_key_id(const internal_key_t & key) const
	{
		key_id_t res = 0;
		key_id_t cur_mult = 1;
		for(unsigned i = 0; i < key.size(); ++i)
		{
			res += cur_mult * key[i];
			cur_mult *= radix;
		}

		return res + total_keys_including_length(key.size() - 1);
	}

	void to_internal(const key_t & alpha_key, internal_key_t & int_key) const
	{
		//int_key.resize(alpha_key.size());
		unsigned s = int_key.size();
		for(unsigned i = 0; i < alpha_key.size(); ++i)
		{
			//int_key[i] = std::distance(alphabet.begin(), std::find(alphabet.begin(), alphabet.end(), alpha_key[i]));
			int_key[s - i - 1] = alphabet.find(alpha_key[i]);
		}
	}

	key_id_t get_key_id(const key_t & alpha_key) const
	{
		internal_key_t ikey(alpha_key.size());
		to_internal(alpha_key, ikey);
		return get_key_id(ikey);
	}

	const key_t alphabet;
	const unsigned radix;
};

template<typename C>
class brute_forcer : public key_manager
{
public:
	using checker_t = C;
	using value_t = typename checker_t::value_t;
	using uint_t = typename key_manager::key_id_t;
	
	brute_forcer(const checker_t & _checker, value_t val, const key_t & _alphabet)
		:key_manager(_alphabet), checker(_checker), correctVal(val)
	{}

	bool operator()(unsigned key_length, uint_t first, uint_t last, key_t & correctKey) const
	{
		internal_key_t cur_key = get_ikey(first);
		//cur_key.resize(key_length, 0);

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


private:
	checker_t checker;
	value_t correctVal;	
};

struct bf_task
{
	bf_task()
		:id(0)
	{}

	friend std::ostream & operator<<(std::ostream & os, const bf_task & bft)
	{
		return os << "#" << bft.id << " length=" << bft.key_length << ", " << big_uint(bft.first) << "->" << big_uint(bft.last);
	}

	unsigned key_length, id;

	key_manager::key_id_t first, last;
};

template<typename B>
struct chunk_generator
{
	using bruteforcer_t = B;
	using uint_t = typename bruteforcer_t::uint_t;

	chunk_generator(const bruteforcer_t & _bf, uint_t start_key = 0, uint_t max_chunk_size = 10000000U)
		:bf(_bf), cur_length(1), cur_idx(start_key), chunkId(1), cur_count(0), maxChunkSize(max_chunk_size)
	{
		cur_length = bf.key_length(cur_idx);
		cur_count = bf.total_keys_including_length(cur_length) - start_key;
		cout << "Creating chunk gen cur_length=" << cur_length << ", (key_id=" << big_uint(start_key) << ", first_key=" << bf.get_ekey(cur_idx) << ", cur_count=" << big_uint(cur_count) << ")" << std::endl;

	}

	bf_task next_chunk()
	{
		std::lock_guard<std::mutex> grd(mtx);

		const uint_t key_count = bf.keys_of_length(cur_length);
		
		uint_t chunk_size = min(key_count - cur_count, maxChunkSize);
		
		bf_task new_task;
		new_task.id = chunkId++;
		new_task.key_length = cur_length;
		new_task.first = cur_idx;
		new_task.last = cur_idx + chunk_size;

		cur_count += chunk_size;
		cur_idx += chunk_size;
 
		//cout << "length=" << cur_length << ", cur_count=" << cur_count << ", key_count=" << key_count << std::endl;
		if(cur_count == key_count)
			++cur_length;

		return std::move(new_task);
	}

	const bruteforcer_t & bf;
	unsigned cur_length;
	uint_t cur_idx, cur_count;
	std::mutex mtx;
	unsigned chunkId;
	const uint_t maxChunkSize; 
};

#endif

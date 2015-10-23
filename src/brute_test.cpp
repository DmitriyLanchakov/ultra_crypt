

#include "brute_forcer.h"
#include "thread_pool.h"
#include <functional>
#include <iostream>
#include <thread>
#include <chrono>
#include <queue>


using namespace std;

struct bf_task
{
	unsigned key_length, id;
	size_t first, last;

	friend std::ostream & operator<<(std::ostream & os, const bf_task & bft)
	{
		return os << "#" << bft.id << " length=" << bft.key_length << ", " << bft.first << "->" << bft.last;
	}
};

template<typename B>
struct chunk_generator
{
	using bruteforcer_t = B;
	using uint_t = typename bruteforcer_t::uint_t;

	chunk_generator(const bruteforcer_t & _bf)
		:bf(_bf), cur_length(1), cur_idx(0), chunkId(0), cur_count(0)
	{

	}

	bf_task next_chunk()
	{
		std::lock_guard<std::mutex> grd(mtx);

		static const uint_t max_chunk_size = 10000000U;
		const uint_t key_count = bf.keys_of_length(cur_length);

		
		uint_t chunk_size = min(key_count - cur_count, max_chunk_size);
		
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
};


template<typename B>
void linear_brute_force(const B & bforcer)
{

	using uint_t = typename B::uint_t;

	uint_t index = 0;

	std::atomic<bool> finished(false);
	std::thread monitor_thread([&](){
		//cout << "Started monitoring thread..." << std::endl;
		while(!finished.load())
		{
			//std::cout << "Current id=" << index << "(" << bforcer.get_ith_key(index) << ")" << std::endl;
			const std::chrono::seconds interval(5);
	        std::this_thread::sleep_for(interval);
	    }
	});

	typename B::key_t correct_key;
	
	chunk_generator<B> gen(bforcer);

	auto start_tp = std::chrono::system_clock::now();

	bool found = false;
	do
	{
		auto cur_chunk = gen.next_chunk();
		cout << "Processing " << cur_chunk << std::endl;
		found = bforcer(cur_chunk.key_length, cur_chunk.first, cur_chunk.last, correct_key);
		index += cur_chunk.last - cur_chunk.first;
	} while(!found);

	auto end_tp = std::chrono::system_clock::now();

	finished.store(true);

	std::chrono::duration<double> diff = end_tp - start_tp;
	cout << "Key found: " << correct_key << "(" << diff.count() << " sec)" << std::endl;

	monitor_thread.join();
}

template<typename B>
class multithread_engine
{
public:
	using bf_t = B;

	multithread_engine(bf_t & _bf)
		:gen(_bf), bf(_bf), found(false)
	{}


	void operator()()
	{
		auto start_tp = std::chrono::system_clock::now();

		for(unsigned i = 0; i < tp.worker_count() + 2; ++i)
			submit_next_chunk();

		tp.wait_finished();

		auto end_tp = std::chrono::system_clock::now();

		std::chrono::duration<double> diff = end_tp - start_tp;
		cout << "Key found: " << this->correctKey << "(" << diff.count() << " sec)" << std::endl;
	}

	void submit_next_chunk()
	{
		tp.submit([&](){
			auto cur_chunk = gen.next_chunk();
			typename B::key_t correct_key;
			cout << "Processing " << cur_chunk << std::endl;
			bool r = bf(cur_chunk.key_length, cur_chunk.first, cur_chunk.last, correct_key);
			if(r)
			{
				this->correctKey = correct_key;
				found.store(true);
			}

			if(!this->found.load())
			{
				this->submit_next_chunk();
			}
		});
	}
private:
	thread_pool tp;
	chunk_generator<B> gen;
	bf_t bf;
	std::atomic<bool> found;
	typename B::key_t correctKey;
};


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


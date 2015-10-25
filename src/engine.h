
#ifndef ULTRA_CRYPT_ENGINE
#define ULTRA_CRYPT_ENGINE

#include "brute_forcer.h"
#include "thread_pool.h"

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
class bf_engine_base
{
public:
	using bf_t = B;
	using uint_t = typename bf_t::uint_t;

	bf_engine_base(bf_t * p_bf)
		:pBF(p_bf)
	{}

	void operator()(uint_t first_key = 0)
	{
		auto start_tp = std::chrono::system_clock::now();
		bool r = search(first_key);
		auto end_tp = std::chrono::system_clock::now();

		if(r)
		{
			std::chrono::duration<double> diff = end_tp - start_tp;
			cout << "Key found: " << this->correctKey << "(" << diff.count() << " sec)" << std::endl;
		}
	}

	virtual bool search(uint_t first_key) = 0;

	typename B::key_t correctKey;
	bf_t * pBF;
};

template<typename B>
class serial_engine : public bf_engine_base<B>
{
public:
	using bf_t = B;
	using _Base = bf_engine_base<B>;
	using uint_t = typename _Base::uint_t;

	serial_engine(bf_t & p_bf)
		:_Base(&p_bf)
	{}

	virtual bool search(uint_t first_key) override
	{
		chunk_generator<bf_t> gen(*this->pBF);

		bool found = false;

		do
		{
			auto cur_chunk = gen.next_chunk();
			found = (*this->pBF)(cur_chunk.key_length, cur_chunk.first, cur_chunk.last, this->correctKey);
			cout << "Processed " << cur_chunk << " (res=" << found << ")" << std::endl;
		}while(!found);

		return true;
	}
};

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
		cout << "Key found: " << this->correctKey << " (" << diff.count() << " sec)" << std::endl;
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


//template<typename T>
//using serial_eng = serial_engine<bf_engine<<T>>>;

//template<typename T>
//using multithread_engine = bf_engine<multithread_engine_base<T>>;

#endif

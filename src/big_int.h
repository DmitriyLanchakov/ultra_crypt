
#ifndef UCRYPT_BIG_INT
#define UCRYPT_BIG_INT

#include <stdint.h>
#include <iostream>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <algorithm>

class big_uint
{
public:
	big_uint(__uint128_t _val)
		:data(_val)
	{}

	big_uint(const std::string & hex_val)
		:data(0)
	{
		std::string hex_str = hex_val;
		std::reverse(hex_str.begin(), hex_str.end());
		hex_str.resize(2 * sizeof(data), '0');
		std::reverse(hex_str.begin(), hex_str.end());
		//std::cout << "parsing " << hex_str << std::endl;

		auto ptr = (unsigned char *)&data;
		//std::string buff(0, 2);
		for(unsigned i = 0; i < sizeof(data); ++i)
		{
			//buff[0] = hex_str[2*i + 1];
			//buff[1] = hex_str[2*i + 0];
			std::string buff = hex_str.substr(2 * i, 2);
			unsigned char val = std::stoi(buff, nullptr, 16);

			//std::cout << '[' << sizeof(data) - i - 1 << ']' << buff << "=>" << (unsigned)val << std::endl;

			ptr[sizeof(data) - i - 1] = val;
		}
	}

	friend bool operator!=(const big_uint & lhs, const big_uint & rhs)
	{
		return lhs.data != rhs.data;
	}

	friend std::ostream & operator<<(std::ostream & os, const big_uint & val)
	{
		std::stringstream ss;

		auto ptr = (unsigned char *)&val.data;

		/*unsigned last_non_zero = sizeof(val.data);
		while((last_non_zero > 1) && (ptr[last_non_zero - 1] == 0))
			++last_non_zero;*/
		bool non_zero = false, first_printed = false;
		for(unsigned i = sizeof(val.data); i > 0; --i)
		{
			if((!non_zero) && (ptr[i - 1] != 0))
				non_zero = true;
			if(non_zero)
			{
				ss << std::hex;

				if(first_printed)
					ss << std::setw(2) << std::setfill('0');
				else
					first_printed = true;

				ss << (unsigned)ptr[i-1];// << ':';
			}
		}
		ss << std::dec;
		if(!non_zero)
			ss << "0";

		return os << ss.str();
	}

	friend std::string to_string(const big_uint & val)
	{
		std::stringstream ss;
		ss << val;
		return ss.str();
	}

	__uint128_t data;
};

#endif

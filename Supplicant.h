#include <stdlib.h>
#include <iostream>
#include <pcap.h>
#include <string.h>
#include <string>
#include "headers.h"
#include "md5.h"

#pragma comment(lib,"ws2_32.lib")

using namespace std;

class Supplicant
{
private:
	pcap_t* fp;
	char errbuf[PCAP_ERRBUF_SIZE];
	u_char destinationMac[6];
	u_char sourceMac[6];
	char *login;
	u_char connectionIdentifier;
	char *challenge;
	char *password;

public:
	Supplicant(){}
	~Supplicant(){}
	
	void init();
	int eapolStart();
	int eapolLogoff();
	int eapResponseIdentify();
	int eapResponseChallenge();
	//void setMac(u_char mac [6]);
	//char* getMac();

};
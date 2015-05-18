#include "Supplicant.h"

using namespace std;

void Supplicant::init()
{

	pcap_if_t *alldevs;
	//pcap_if_t *d;
	int inum;
	int i = 0;
	pcap_t *adhandle;
	char errbuf[PCAP_ERRBUF_SIZE];

	/* Retrieve the device list on the local machine */
	if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &alldevs, errbuf) == -1)
	{
		fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
		exit(1);
	}

	/* Print the list */
	for (d = alldevs; d; d = d->next)
	{
		printf("%d. %s", ++i, d->name);
		if (d->description)
			printf(" (%s)\n", d->description);
		else
			printf(" (No description available)\n");
	}

	if (i == 0)
	{
		printf("\nNo interfaces found! Make sure WinPcap is installed.\n");
		return;
	}

	printf("Enter the interface number (1-%d):", i);
	scanf_s("%d", &inum);

	if (inum < 1 || inum > i)
	{
		printf("\nInterface number out of range.\n");
		/* Free the device list */
		pcap_freealldevs(alldevs);
		return;
	}

	/* Jump to the selected adapter */
	for (d = alldevs, i = 0; i < inum - 1; d = d->next, i++);


	if ((fp = pcap_open(d->name,    // name of the device
		100,						// portion of the packet to capture (only the first 100 bytes)
		PCAP_OPENFLAG_PROMISCUOUS,  // promiscuous mode
		1000,						// read timeout
		NULL,						// authentication on the remote machine
		errbuf						// error buffer
		)) == NULL)
		;

	for (int i = 0; i < 6; ++i)
		sourceMac[i] = (u_char)0x20;
	
	cout << "Adres MAC " << hex << (int)d->addresses->addr->sa_data[0] << (int)d->addresses->addr->sa_data[1] << (int)d->addresses->addr->sa_data[2] << (int)d->addresses->addr->sa_data[3] << (int)d->addresses->addr->sa_data[4] << endl << endl << endl;
	printf("%.2X\n", d->addresses->addr->sa_data[2]);
	connectionIdentifier = 0x0;		// TODO: ARAP MUSI WYZNACZYC, A MY MUSIMY TO ODEBRAC!

	//login = "Maciek";
	//password = "pass";
	//challenge = "chal";
	setChallenge("CHALLENGE");
	setLogin("Maciek");
	setPassword("PASSWORD");
	sessionActive = 1;
	packetCounter = 0;
	lastIdentifier = (u_char)0x40;

}

void Supplicant::setChallenge(string cha)
{
	/*for (int i = 0; i < strlen(chal); ++i)
		challenge[i] = chal[i];*/
	challenge = cha;
}

void Supplicant::setLogin(string log)
{
	login = log;
}

void Supplicant::setPassword(string pas)
{
	password = pas;
}

int Supplicant::eapolStart()
{
	u_char packet_buffer[100];
	ETHERNET_HEADER* eth;

	eth = (ETHERNET_HEADER*)packet_buffer;
	memcpy(eth->destination, destinationMac, 6);
	memcpy(eth->source, sourceMac, 6);
	eth->type = htons(0x888E);
	eth->protocol_version = 2;
	eth->packet_type = 1;
	eth->packet_body_length = 0;

	if (pcap_sendpacket(fp, packet_buffer, sizeof(ETHERNET_HEADER)) != 0)
	{
		fprintf(stderr, "\nError sending the EAPOL-Start packet: \n", pcap_geterr(fp));
		return 1;
	}

	return 0;
}


int Supplicant::eapolLogoff()
{
	u_char packet_buffer[100];
	ETHERNET_HEADER* eth;

	eth = (ETHERNET_HEADER*)packet_buffer;
	memcpy(eth->destination, destinationMac, 6);
	memcpy(eth->source, sourceMac, 6);
	eth->type = htons(0x888E);
	eth->protocol_version = 2;
	eth->packet_type = 2;
	eth->packet_body_length = 0;

	if (pcap_sendpacket(fp, packet_buffer, sizeof(ETHERNET_HEADER)) != 0)
	{
		fprintf(stderr, "\nError sending the EAPOL-Start packet: \n", pcap_geterr(fp));
		return 1;
	}

	return 0;
}



int Supplicant::eapResponseIdentify()
{
	u_char packet_buffer[100];
	ETHERNET_HEADER* eth;
	EAP_HEADER* eap;

	eth = (ETHERNET_HEADER*)packet_buffer;
	memcpy(eth->destination, destinationMac, 6);
	memcpy(eth->source, sourceMac, 6);
	eth->type = htons(0x888E);
	eth->protocol_version = 2;
	eth->packet_type = 0;
	eth->packet_body_length = sizeof(EAP_HEADER) + login.length();

	eap = (EAP_HEADER*)(packet_buffer + sizeof(ETHERNET_HEADER));
	eap->code = 2;
	eap->identifier = lastIdentifier;
	eap->length = htons(0x0006);

	//cout << strlen(login) << endl;

	char* data;
	data = (char *)(packet_buffer + sizeof(ETHERNET_HEADER) + sizeof(EAP_HEADER));
	*data = 0x1;
	data = (char*)(packet_buffer + sizeof(ETHERNET_HEADER) + sizeof(EAP_HEADER) + 1);
	strcpy_s(data, login.length() + 1, login.c_str());

	cout << sizeof(ETHERNET_HEADER) + sizeof(EAP_HEADER) + login.length() + 1 << endl; //TODO sprawdzi� czy �miga

	if (pcap_sendpacket(fp, packet_buffer, sizeof(ETHERNET_HEADER) + sizeof(EAP_HEADER) + login.length() + 1) != 0)
	{
		fprintf(stderr, "\nError sending the EAPOL-Start packet: \n", pcap_geterr(fp));
		return 1;
	}

	return 0;
}

int Supplicant::eapResponseChallenge()
{
	string res = (char)lastIdentifier + password + challenge;

	cout << "res:" << res << endl;

	const char *res2 = res.c_str();

	MD5 md5;
	md5.add(res2, res.length());
	cout << md5.getHash() << " skrot" << endl;	// tworzenie skrotu opisane w RFC 1994!

	const char *res3 = (md5.getHash()).c_str();

	ETHERNET_HEADER* eth;
	EAP_HEADER* eap;
	u_char packet_buffer[100];

	eth = (ETHERNET_HEADER*)packet_buffer;
	memcpy(eth->destination, destinationMac, 6);
	memcpy(eth->source, sourceMac, 6);
	eth->type = htons(0x888E);
	eth->protocol_version = 2;
	eth->packet_type = 0;
	eth->packet_body_length = htons ( sizeof(EAP_HEADER) + 18 + login.length()); // 1 - type , 16 - skrot md5

	eap = (EAP_HEADER*)(packet_buffer + sizeof(ETHERNET_HEADER));
	eap->code = 2;
	eap->identifier = lastIdentifier;
	eap->length =htons( (sizeof(EAP_HEADER) + 18 + login.length() ));
	cout << "EAP size: " << sizeof(EAP_HEADER) << endl;
	cout << login.length() << endl;
	cout << "Len: " << (sizeof(EAP_HEADER) + 18 + login.length()) << endl;

	char *data;
	data = (char *)(packet_buffer + sizeof(ETHERNET_HEADER) + sizeof(EAP_HEADER));
	*data = 0x4;
	data = (char *)(packet_buffer + sizeof(ETHERNET_HEADER) + sizeof(EAP_HEADER) + 1);
	int valueSize = 16 + login.length();
	*data = valueSize;
	++data;

	/*
	for (int i = 0; i < 16; ++i)
		*(data + i) = md5.getHash()[i];
	*/
	u_char temp[17];
	//for (int i = 0; i < 32; ++i)
		//temp[i] = md5.getHash()[i];
	//string temp2;
	md5.getHash(temp);
	/*
	printf("MD5:");
	for (int i = 0; i < 16; ++i)
		printf(" %.2X", temp[i]);
	printf("\n");
	*/
	//temp2 = convertToHex(temp);
	//cout <<"MD12:" <<temp2<<endl;

	cout << " Wysylany hash: " << endl;
	for (int i = 0; i < 16; ++i)
	{
		*(data + i) = temp[i];
		cout << temp[i];
	}
	data = data + 16;
	for (int i = 0; i < login.length(); ++i)
		*(data + i) = login[i];



	if (pcap_sendpacket(fp, packet_buffer, sizeof(ETHERNET_HEADER) + sizeof(EAP_HEADER) + 18 + login.length()) != 0)
	{
		fprintf(stderr, "\nError sending the EAP-MD5-Challenge Response packet: \n", pcap_geterr(fp));
		return 1;
	}

	return 0;
}

int Supplicant::listen()
{
	char packet_filter[] = "ether proto 0x888E";	//magiczny filtr dopuszczajacy tylko eapol
	struct bpf_program fcode;

	cout << "!!";

	u_int netmask;
	if (d->addresses != NULL)
		/* Retrieve the mask of the first address of the interface */
		netmask = ((struct sockaddr_in *)(d->addresses->netmask))->sin_addr.S_un.S_addr;
	else
		/* If the interface is without addresses we suppose to be in a C class network */
	{
		netmask = 0xffffff; 
	}


	if (pcap_compile(fp, &fcode, packet_filter, 1, netmask) < 0)
	{
		fprintf(stderr, "\nUnable to compile the packet filter. Check the syntax.\n");


		return -1;
	}

	//set the filter
	if (pcap_setfilter(fp, &fcode) < 0)
	{
		fprintf(stderr, "\nError setting the filter.\n");

		return -1;
	}


	printf("\nlistening on %s...\n", d->description);
	
	//pcap_loop(fp, 0, Listener::packetListener, NULL);
	return 0;
}

void Supplicant::listenNext()
{
	bool breaker = true;
	char packet_filter[] = "ether proto 0x888E";	//magiczny filtr dopuszczajacy tylko eapol
	struct bpf_program fcode;

	cout << "!!";

	u_int netmask;
	if (d->addresses != NULL)
		/* Retrieve the mask of the first address of the interface */
		netmask = ((struct sockaddr_in *)(d->addresses->netmask))->sin_addr.S_un.S_addr;
	else
		/* If the interface is without addresses we suppose to be in a C class network */
	{
		netmask = 0xffffff;
	}


	if (pcap_compile(fp, &fcode, packet_filter, 1, netmask) < 0)
	{
		fprintf(stderr, "\nUnable to compile the packet filter. Check the syntax.\n");


		return;
	}

	//set the filter
	if (pcap_setfilter(fp, &fcode) < 0)
	{
		fprintf(stderr, "\nError setting the filter.\n");

		return;
	}
	struct pcap_pkthdr *header; 
	const u_char *pkt_data;
	while (breaker)
	{
		int res;
		res = pcap_next_ex(fp, &header, &pkt_data);
		struct tm ltime;
		char timestr[16];
		time_t local_tv_sec;
		u_char temp[100];

		if (res == 0)
			continue;

		++packetCounter;
		string log("");
		log += getDestinationMac();
		log += " ";
		log += getSourceMac();// TODO  ZROBIC KONWERSJE Z UCHAR NA HEX!!!!
		log += " ";

		int len = header->len;
		cout << "Packet Received: (nr:" << packetCounter << ")" << len << endl;
		log += "Packet Received: (nr:";
		log += packetCounter;
		log += ")";

		for (int i = 0; i < len; ++i)
		{
			temp[i] = *(pkt_data + i);
		}
		cout << endl;

		ETHERNET_HEADER* eth;
		EAP_HEADER* eap;
		char* data;
		//data = (char*)(temp + sizeof(ETHERNET_HEADER)+sizeof(EAP_HEADER));
		eth = (ETHERNET_HEADER*)temp;
		//cout << eth -> packet_type<< endl;
		//printf("%.2X", eth->packet_type);

		switch (eth->packet_type){
		case 0x00:
			cout << "packet" << endl;
			log += " packet EAP-Packet,";
			eap = (EAP_HEADER*)(temp + sizeof(ETHERNET_HEADER));

			lastIdentifier = eap->identifier;

			switch (eap->code){
			case 0x01:
				cout << "req" << endl;
				log += " request/";
				data = (char*)(temp + sizeof(ETHERNET_HEADER)+sizeof(EAP_HEADER));
				cout << "type:" << endl;
				if (*data == (char)0x1)
				{
					cout << "Identify" << endl;
					log += "identify";
					data = (char*)(temp + 6);
					for (int i = 0; i < 6; ++i)
						destinationMac[i] = (u_char)*(data + i);

					eapResponseIdentify();
					//Zawsze jak chcesz wyslac pakiet to breaker = 0 ; to wylaczy listener i dzieki temu nie odczytamy wlasnego pakietu.
					breaker = 0;
				}

				//TODO inne pakiety ? Notification(0x2)? Nak(0x3) ?

				if (*data == (char)0x4)
				{
					cout << "MD5_Challenge" << endl;
					log += "MD5-Challenge";
					//data += 2; ciekawe czy tak moge.
					data = (char*)(temp + sizeof(ETHERNET_HEADER)+sizeof(EAP_HEADER)+1);
					int valueSize = *data;
					cout << valueSize << " valueSize" << endl;
					// [type(1)][value_size(1)!!][value][name] // TODO sprawdzic(3.4 RFC);
					char temp2[100];
					cout << "poczatkowa temp2 " << strlen(temp2) << endl;
					++data;
					string jakis("");
					for (int i = 0; i < valueSize; ++i)
					{
						jakis += (char)*(data + i);
					}
					cout << "jakis " << jakis << " dlugosc " << jakis.length()<< endl;
					setChallenge(jakis);

					eapResponseChallenge(); 
					//wylacz listener
					breaker = 0;
				}


				printf("%.2X", *data);
				break;
			case 0x02:
				//illegal packet

				/*
				cout << "respo" << endl;
				data = (char*)(temp + sizeof(ETHERNET_HEADER)+sizeof(EAP_HEADER));
				cout << "type:" << endl;
				if (*data == (char)0x1)
				cout << "JES" << endl;
				printf("%.2X", *data);
				eapResponseIdentify();
				//eapolLogoff();
				breaker = 0;
				cout << endl;
				*/
				break;

			case 0x03:
				cout << "Success: Client authentication accepted. Access to network granted. " << endl;
				log += " success";
				//Start console
				//Start packet transmission
				//when done:
				// sessionActive = 0;
				breaker = 0;
				break;
			case 0x04:
				cout << "fail" << endl;
				log += " fail";
				break;
			default:
				cout << "code: def" << endl;
				log += " uknown EAP code";
			}

			break;
		case 0x01:
			cout << "start" << endl;
			log += " EAPOL-Start";
			break;
		case 0x02:
			cout << "logoff" << endl;
			log += " EAPOL-Logoff";
			break;
		default:
			cout << "type: unknown  " << endl;
			log += " unknown type";
		}
		log += "\n";
		cout << "LOG : " << log << endl;
		logger << log;
	}
}

string Supplicant::getDestinationMac()
{
	string addr;
	addr = "";

	for (int i = 0; i<6; ++i){

		char buffer[50];
		sprintf_s(buffer, "%.2X", destinationMac[i]);
		string str(buffer);
		addr += str;
		if (i != 5)
			addr += ":";
	}

	return addr;
}

string Supplicant::convertToHex(u_char* t)
{
	string out;
	out = "";

	for (int i = 0; i<32; ++i){

		char buffer[50];
		sprintf_s(buffer, "%.2X", t[i]);
		string str(buffer);
		out += str;
	}

	return out;
}

string Supplicant::getSourceMac()
{
	string addr;
	addr = "";

	for (int i = 0; i<6; ++i){

		char buffer[50];
		sprintf_s(buffer, "%.2X", sourceMac[i]);
		string str(buffer);
		addr += str;
		if (i != 5)
			addr += ":";
	}

	return addr;
}



#include "Poco/Net/StreamSocket.h"
#include <iostream>
#include <string>
#include <fstream>

#include "Buffer.h"

using Poco::Net::Socket;
using Poco::Net::StreamSocket;
using Poco::Net::SocketAddress;

using namespace Protobuf_net;
using namespace std;

unsigned short port = 5001;
string serverAddr = "localhost";
string filename = "1.info";

enum OptionCode
{
	findUserInfo = 0,
	addUser = 1,
	delUser = 2,
	updateUser = 3,
	validateUser = 4
};

#define FINGER_LEN  2048

int main(int argc, char** argv)
{
	if (argc > 1)
		serverAddr = string(argv[1]);
	if (argc > 2)
		port = atoi(argv[2]);
	if (argc > 3)
		filename = argv[3];

	Buffer	buffer;

	int len = 0;
	int totalLen = 0;

	char  code = validateUser;

	buffer.append((const char*)&code, 1);
	totalLen = totalLen + 1;

	std::ifstream ifs(filename.c_str(), std::ifstream::binary | std::ifstream::in);
	if (ifs.is_open())
	{
		if (ifs.good())
		{
			ifs.seekg(0, ifs.end);
			len = ifs.tellg();
			ifs.seekg(0, ifs.beg);

			buffer.append((const char*)&len, 4);
			unsigned char bin[FINGER_LEN] = { 0 };
			ifs.read((char*)bin, len);
			buffer.append((const char*)bin, len);
			totalLen = totalLen + 4 + len;
		}
	}
	else
	{
		cout << "finger file " << filename << " is not OK!" << endl;
		return -2;
	}

	Buffer sendBuf;
	sendBuf.append((const char*)&totalLen, 4);
	sendBuf.append(buffer.retrieveAllAsString());

	int sendLen = sendBuf.readableBytes();
	
	StreamSocket ss;
	try
	{
		ss.connect(SocketAddress(serverAddr, port));
	}
	catch (Poco::Exception& exc)
	{
		cerr << "Connect server failed, err: " << exc.displayText() << endl;
		return -1;
	}

	ss.sendBytes(sendBuf.retrieveAllAsString().c_str(), sendLen);

	char buf[FINGER_LEN] = { 0 };
	int n = ss.receiveBytes(buf, sizeof(buf));

	std::copy(buf, buf + 4, (char*)&totalLen);
	std::copy(buf + 4, buf + 5, &code);
	if (code == 0)
	{
		char *p = buf;
		p += 5;

		char tmp[32] = { 0 };

		int nameIdLen = 0;
		std::copy(p, (char*)(p + 4), (char*)&nameIdLen);
		p += 4;

		string nameId;
		nameId.resize(nameIdLen);
		std::copy(p, (char*)(p + nameIdLen), nameId.begin());
		p += nameIdLen;

		memset(tmp, 0, 32);
		int passwdLen = 0;
		std::copy(p, p + 4, (char*)&passwdLen);
		p += 4;

		string passwd;
		passwd.resize(passwdLen);
		std::copy(p, (char*)(p + passwdLen), passwd.begin());

		cout << "nameId: " << nameId << ", passwd: " << passwd << endl;

	}
	cout << "totalLen: " << totalLen << ", code: " << (int)code << endl;
	
	//char c(' ');

	//while (c != 'q' && c != 'Q')
	//{
	//	cout << "Press q then enter to quit: \n";
	//	cin >> c;
	//}
	return 0;
}
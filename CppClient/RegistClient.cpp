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

unsigned short port = 5000;
string serverAddr = "localhost";
string filename = "1.info";

enum OptionCode
{
	queryUserInfo = 0,
	addUser = 1,
	delUser = 2,
	updateUser = 3,
	validateUser = 4,
	queryAllUserInfo = 5
};

#define FINGER_LEN  2048

StreamSocket ss;

int testOption(char option, const string userId = "", const string passwd = "", const string filename = "")
{
	Buffer	buffer;
	int code = option;

	int len = 0;
	int totalLen = 0;
	

	buffer.append((const char*)&code, 1);
	totalLen = totalLen + 1;
	len = userId.length();
	buffer.append((const char*)&len, 4);
	buffer.append(userId);
	totalLen = totalLen + 4 + len;

	if (code == addUser || code == updateUser)
	{
		len = passwd.length();
		buffer.append((const char*)&len, 4);
		buffer.append(passwd);
		totalLen = totalLen + 4 + len;


		std::ifstream ifs(filename.c_str(), std::ifstream::binary | std::ifstream::in);
		if (ifs.is_open())
		{
			if (ifs.good())
			{
				ifs.seekg(0, ifs.end);
				len = ifs.tellg();
				ifs.seekg(0, ifs.beg);
				unsigned char bin[FINGER_LEN] = { 0 };
				buffer.append((const char*)&len, 4);
				
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
	}
	Buffer sendBuf;
	sendBuf.append((const char*)&totalLen, 4);
	sendBuf.append(buffer.retrieveAllAsString());

	int sendLen = sendBuf.readableBytes();

	ss.sendBytes(sendBuf.retrieveAllAsString().c_str(), sendLen);

	char buf[2048] = { 0 };
	int n = ss.receiveBytes(buf, sizeof(buf));
	std::copy((char*)buf, (char*)(buf + 4), (char*)&totalLen);
	std::copy((char*)(buf + 4), (char*)(buf + 5), &code);

	cout << "totalLen: " << totalLen << ", code: " << (int)code << endl;

	if (option == queryUserInfo)
	{
		int useridlen = 0;
		int lenPlus = 5;
		std::copy((char*) (buf + lenPlus), (char*) (buf + lenPlus + 4), (char*) &useridlen);
		string userid;
		userid.resize(useridlen);
		lenPlus = 9;
		std::copy((char*) (buf + lenPlus), (char*) (buf + lenPlus + useridlen), &(*userid.begin()));

		int passwdlen = 0;
		lenPlus = 5 + 4 + useridlen;
		std::copy((char*) (buf + lenPlus), (char*) (buf + lenPlus + 4), (char*) &passwdlen);
		string passwd;
		lenPlus = 5 + 4 + useridlen + 4;
		passwd.resize(passwdlen);
		std::copy((char*) (buf + lenPlus), (char*) (buf + lenPlus + passwdlen), &(*passwd.begin()));

		int fingerLen = 0;
		lenPlus = 5 + 4 + useridlen + 4 + passwdlen;
		std::copy((char*) (buf + lenPlus), (char*) (buf + lenPlus + 4), (char*) &fingerLen);
		char finger[2048] = {0};
		lenPlus = 5 + 4 + useridlen + 4 + passwdlen + 4; 
		std::copy((char*) (buf + lenPlus), (char*) (buf + lenPlus + fingerLen), (char*) finger);

		cout << "useridlen: " << useridlen << ", userid: " << userid << ", passwdlen: " << passwdlen << ", passwd: " << passwd << ", fingerLen" << fingerLen << endl;

		unsigned char bin[FINGER_LEN] = { 0 };
		std::ifstream ifs(filename.c_str(), std::ifstream::binary | std::ifstream::in);
		if (ifs.is_open())
		{
			if (ifs.good())
			{
				ifs.seekg(0, ifs.end);
				len = ifs.tellg();
				ifs.seekg(0, ifs.beg);

				ifs.read((char*) bin, len);
			}
		}

		if (memcmp(bin, finger, fingerLen ) == 0)
			cout << "finger data ok!" << endl;
	}
	else if (option == queryAllUserInfo)
	{
		int userLen1 = 0;
		std::copy((char*) (buf + 5), (char*) (buf + 9), (char*)&userLen1);
		char user1[32] = {0};
		std::copy((char*) (buf + 9), (char*) (buf + 9 + userLen1), user1);

		int userLen2 = 0;
		std::copy((char*) (buf + 9 + userLen1), (char*) (buf + 13 + userLen1), (char*) &userLen2);
		char user2[32] = {0};
		std::copy((char*) (buf + 13 + userLen1), (char*) (buf + 13 + userLen1 + userLen2), user2);

		cout << "user1: " << user1 << ", user2" << user2 << endl;

	}
	return 0;
}

int main(int argc, char** argv)
{
	if (argc > 1)
		serverAddr = string(argv[1]);
	if (argc > 2)
		port = atoi(argv[2]);
	if (argc > 3)
		filename = string(argv[3]);

	try
	{
		ss.connect(SocketAddress(serverAddr, port));
	}
	catch (Poco::Exception& exc)
	{
		cout << "Connect server failed, err: " << exc.displayText() << endl;
		return -1;
	}
	testOption(addUser, "1011", "1234567890!@#$%^&*~1234567890!@#$%^&*~abcdefghijklmnopqrstHIHIHHKJDHSKJHDQOPMMXZSD1234567890!@#$%^&*~abc", filename);
	testOption(addUser, "2011", "2011b", filename);
	testOption(queryUserInfo, "1234567890!@#$%^&*~1234567890!@#$%^&*~abcdefghijklmnopqrstHIHIHHKJDHSKJHDQOPMMXZSD", "", filename);
	testOption(queryUserInfo, "1011", "", filename);
	testOption(queryAllUserInfo);
	testOption(updateUser, "1011", "1234567890!@#$%^&*~abcdefghijklmnopqrstHIHIHHKJDHSKJHDQOPMMXZSD", filename);
	testOption(delUser, "1011");

	char c(' ');

    /*
	while (c != 'q' && c != 'Q')
	{
	cout << "Press q then enter to quit: \n";
	cin >> c;
	}
	*/
	return 0;
}
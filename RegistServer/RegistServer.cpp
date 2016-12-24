#include "Buffer.h"
#include "CppSQLite3.h"
#include "ComDef.h"
#include "DBHelper.h"

#include "Poco/Net/SocketReactor.h"
#include "Poco/Net/SocketAcceptor.h"
#include "Poco/Net/SocketNotification.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/NetException.h"
#include "Poco/NObserver.h"
#include "Poco/Exception.h"
#include "Poco/Thread.h"
#include "Poco/Mutex.h"
#include <iostream>
#include <vector>



using Poco::Net::SocketReactor;
using Poco::Net::SocketAcceptor;

using Poco::Net::SocketNotification;
using Poco::Net::ReadableNotification;
using Poco::Net::ShutdownNotification;
using Poco::Net::ServerSocket;
using Poco::Net::StreamSocket;
using Poco::NObserver;
using Poco::AutoPtr;
using Poco::Thread;
using Poco::FastMutex;

using namespace Protobuf_net;
using namespace std;


const int kIntLen = sizeof(int);


struct Client
{
	Buffer			inputBuffer;
	Buffer			outputBuffer;
	StreamSocket	socket;
};

DBHelper _dbhelper;
Poco::FastMutex s_mutex;

class RegistServiceHandler;
RegistServiceHandler *g_preuser = NULL;

class RegistServiceHandler
{
public:
	RegistServiceHandler(StreamSocket& socket, SocketReactor& reactor)
		:_reactor(reactor)
	{
		cout << "has a new connection!" << endl;

		s_mutex.lock();
		if (g_preuser != NULL)
		{
			cout << "delete previous user" << endl;
			delete g_preuser;
			g_preuser = NULL;
		}
		_user.socket = socket;
		g_preuser = this;
		s_mutex.unlock();

		_reactor.addEventHandler(_user.socket, NObserver<RegistServiceHandler, ReadableNotification>(*this, &RegistServiceHandler::onReadable));
		_reactor.addEventHandler(_user.socket, NObserver<RegistServiceHandler, ShutdownNotification>(*this, &RegistServiceHandler::onShutdown));

	}

	~RegistServiceHandler()
	{
		_reactor.removeEventHandler(_user.socket, NObserver<RegistServiceHandler, ReadableNotification>(*this, &RegistServiceHandler::onReadable));
		_reactor.removeEventHandler(_user.socket, NObserver<RegistServiceHandler, ShutdownNotification>(*this, &RegistServiceHandler::onShutdown));
	}

	void onReadable(const AutoPtr<ReadableNotification>& pNf)
	{
		try
		{
			int n = _user.inputBuffer.receiveBytes(_user.socket);
			if (n <= 0)
			{
				cout << "delete this" << endl;
				s_mutex.lock();
				if (g_preuser == this) g_preuser = NULL;
				delete this;
				s_mutex.unlock();
			}
		}
		catch (Poco::Exception& exc)
		{
			cerr << "[RegistServiceHandler][onReadable]: " << exc.displayText() << endl;

			// delete the disconnected-user connection
			s_mutex.lock();
			if (g_preuser == this) g_preuser = NULL;
			delete this;
			s_mutex.unlock();
		}

		onBufferMessage();
	}


	void internalSendMessage()
	{
		int sent = 0;
		while (_user.outputBuffer.readableBytes() > 0)
		{
			sent = _user.socket.sendBytes(_user.outputBuffer.peek(), _user.outputBuffer.readableBytes());
			_user.outputBuffer.retrieve(sent);
		} 
	}

	void doDataAnalyze()
	{
		int ret = inputDataError;
		Message msg;
		Message dst;
		list<string> passwdList;
		char buf[1024] = { 0 };

		_user.inputBuffer.retrieve((char*) &msg.option, 1);

		_user.inputBuffer.retrieve((char*) &msg.userIdLen, 4);

		if (msg.userIdLen > 50) // max len is 50
		{
			cerr << "userId string is too long!" << endl;
			ret = inputDataError; goto END;
		}

		_user.inputBuffer.retrieve(buf, msg.userIdLen);
		msg.userId = string(buf);

		if (msg.option == addUser || msg.option == updateUser)
		{
			_user.inputBuffer.retrieve((char*) &msg.passwdLen, 4);
			if (msg.passwdLen > 399) // max len is 399
			{
				cout << "passwd string is too long!"<< endl;
				ret = inputDataError; goto END;
			}

			memset(buf, 0, sizeof buf);
			_user.inputBuffer.retrieve(buf, msg.passwdLen);
			msg.passwd = string(buf);

			_user.inputBuffer.retrieve((char*) &msg.fingerLen, 4);
			if (msg.fingerLen > FINGGER_BUF) // 4096
			{
				cerr << "fingerLen data length is too long!"<< endl;
				ret = inputDataError; goto END;
			}
			else if (msg.fingerLen < 128)
			{
				cerr << "fingerLen data length is too long!"<< endl;
				ret = inputDataError; goto END;
			}
			memset(msg.finger, 0, FINGGER_BUF);
			_user.inputBuffer.retrieve((char*) msg.finger, msg.fingerLen);
		}

		switch (msg.option)
		{
		case addUser:
			cout << "has a new addUser request!"<< endl;
			ret = _dbhelper.addUser(msg);
			break;
		case delUser:
			cout << "has a new delUser request!"<< endl;
			ret = _dbhelper.delUser(msg);
			break;
		case updateUser:
			cout << "has a new updateUser request!"<< endl;
			ret = _dbhelper.updateUser(msg);
			break;
		case queryUserInfo:
			cout << "has a queryUserInfo request!"<< endl;
			ret = _dbhelper.findUser(msg.userId, dst);
			break;
		case queryAllUserInfo:
			cout << "has a queryAllUserInfo request!"<< endl;
			ret = _dbhelper.findAllUser(passwdList);
			break;
		default:
			cerr << "has a error request!, option is " << (char) msg.option<< endl;
		}

	END:
		char code = (char) ret;
		string result;
		result.resize(kIntLen);	//Reserved 4 bytes for head


		int totalLen = 1;
		result.append(1, code);
		if (code == success)
		{
			if (msg.option == queryUserInfo)
			{
				result.append((char*) &dst.userIdLen, 4);
				result.append(dst.userId);
				totalLen = totalLen + 4 + dst.userIdLen;

				result.append((char*) &dst.passwdLen, 4);
				result.append(dst.passwd);
				totalLen = totalLen + 4 + dst.passwdLen;

				result.append((char*) &dst.fingerLen, 4);
				result.append((char*) dst.finger, dst.fingerLen);
				totalLen = totalLen + 4 + dst.fingerLen;
			}
			else if (msg.option == queryAllUserInfo)
			{
				for (std::list<string>::iterator it = passwdList.begin(); it != passwdList.end(); ++it)
				{
					string str = *it;
					int len = str.length();
					result.append((char*) &len, 4);
					result.append(str);
					totalLen = totalLen + 4 + len;
				}
			}
		}

		std::copy((char*) &totalLen, (char*) &totalLen + 4, result.begin());

		_user.outputBuffer.append(result);
		internalSendMessage();
		cout << "reply sent! code: " << (int) code << endl;
	}

	void onBufferMessage()
	{
		const int kIntLen = sizeof(int);
		while (_user.inputBuffer.readableBytes() >= kIntLen)
		{
			const void* data = _user.inputBuffer.peek();
			const int* tmp = static_cast<const int*>(data);
			int len = *tmp;
			if (len > 65536 || len < 0)
			{
				// "Invalid length "
				cout << "Invalid receive message length " << endl;
				_user.inputBuffer.retrieveAll();
				break;
			}
			else if (_user.inputBuffer.readableBytes() >= len + kIntLen)
			{
				_user.inputBuffer.retrieve(kIntLen);
				s_mutex.lock();
				doDataAnalyze();
				s_mutex.unlock();
				// message handle
			}
			else
			{
				break;
			}
		}
	}

	void onShutdown(const AutoPtr<ShutdownNotification>& pNf)
	{
		cout << "[RegistServiceHandler][onShutdown]" << endl;
		delete this;
	}

private:
	SocketReactor& _reactor;
	Client _user;
};



int main(int argc, char** argv)
{	
	if (argc < 2)
	{
		cout << "useage : app <dbPath> <port-(optinal)>" << endl;
		return 0;
	}
	string dbPath = string(argv[1]);
	unsigned short port = 5000; // maybe default port is 5000
	if (argc > 2)
		port = atoi(argv[2]);

	_dbhelper.init(dbPath);
	// set-up a server socket
	ServerSocket svs(port);
	// set-up a SocketReactor...
	SocketReactor reactor;
	// ... and a SocketAcceptor
	SocketAcceptor<RegistServiceHandler> acceptor(svs, reactor);
	// run the reactor in its own thread so that we can wait for 
	// a termination request
	Thread thread;
	thread.start(reactor);

	cout << "RegistServer started!, port: " << port << ", dbPath: " << dbPath << endl;

	char c(' ');

	while (c != 'q' && c != 'Q')
	{
		cout << "Press q then enter to quit: \n";
		cin >> c;
	}
	// Stop the SocketReactor
	reactor.stop();
	thread.join();

	return 0;
}

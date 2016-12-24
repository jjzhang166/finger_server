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
	Buffer			inputBuffer;	// input message queue
	Buffer			outputBuffer;	// output message queue
	StreamSocket	socket;			// socket handler
};

DBHelper _dbhelper;

class ValiateServiceHandler
{
public:
	ValiateServiceHandler(StreamSocket& socket, SocketReactor& reactor)
		:_reactor(reactor)
	{
		_user.socket = socket;
		_reactor.addEventHandler(_user.socket, NObserver<ValiateServiceHandler, ReadableNotification>(*this, &ValiateServiceHandler::onReadable));
		_reactor.addEventHandler(_user.socket, NObserver<ValiateServiceHandler, ShutdownNotification>(*this, &ValiateServiceHandler::onShutdown));
		cout << "has a new connection!" << endl;
	}

	~ValiateServiceHandler()
	{
		_reactor.removeEventHandler(_user.socket, NObserver<ValiateServiceHandler, ReadableNotification>(*this, &ValiateServiceHandler::onReadable));
		_reactor.removeEventHandler(_user.socket, NObserver<ValiateServiceHandler, ShutdownNotification>(*this, &ValiateServiceHandler::onShutdown));
	}

	void onReadable(const AutoPtr<ReadableNotification>& pNf)
	{
		try
		{
			int n = _user.inputBuffer.receiveBytes(_user.socket);
			if (n <= 0)
			{
				cout << "delete this" << endl;
				delete this;
			}
		}
		catch (Poco::Exception& exc)
		{
			cerr << "[ValiateServiceHandler][onReadable]: " << exc.displayText() << endl;

			// delete the disconnected-user connection
			delete this;
		}

		onBufferMessage();
	}


	void internalSendMessage()
	{
		int sended = 0;
		while (_user.outputBuffer.readableBytes() > 0)
		{
			sended = _user.socket.sendBytes(_user.outputBuffer.peek(), _user.outputBuffer.readableBytes());
			_user.outputBuffer.retrieve(sended);
		} 
	}

	void doDataAnalyze()
	{
		Message msg;
		string userId;
		string passwd;
		string result;
		int ret = inputDataError;
		result.resize(kIntLen);	//Reserved 4 bytes for head

		_user.inputBuffer.retrieve((char*)&msg.option, 1);

		if (msg.option != validateUser)
		{
			cout << "has a error request!, option is " << (char*)msg.option << endl;
			ret = inputDataError; goto END;
		}
		else
		{
			cout << "has a new validateUser request!" << endl;

			_user.inputBuffer.retrieve((char*)&msg.fingerLen, 4);
			if (msg.fingerLen > FINGGER_BUF) // 4096
			{
				cout << "fingerLen data length is too long!" << endl;
				ret = inputDataError; goto END;
			}
			else if (msg.fingerLen < 128) 
			{
				cout << "fingerLen data length is too long!" << endl;
				ret = inputDataError; goto END;
			}

			memset(msg.finger, 0, FINGGER_BUF);
			_user.inputBuffer.retrieve((char*)msg.finger, msg.fingerLen);

			
			
			ret = _dbhelper.validateUser(userId, passwd, msg.finger, msg.fingerLen);
		}
END:
		int totalLen = 0;
		char code = (char)ret;

		if (ret == 0)
		{
			cout << "validateUser ok!" << endl;

			result.append(1, code);
			totalLen += 1;

			int len = 0;

			len = userId.length();
			result.append((char*)&len, 4);
			result.append(userId);
			totalLen = totalLen + 4 + len;

			len = passwd.length();
			result.append((char*)&len, 4);
			result.append(passwd);
			totalLen = totalLen + 4 + len;
		}
		else
		{
			cout << "validateUser ERROR!" << endl;

			result.append(1, code);
			totalLen += 1;
		}

		std::copy((char*)&totalLen, (char*)&totalLen + 4, result.begin());

		_user.outputBuffer.append(result);
		internalSendMessage();
		cout << "reply sent! code: " << (int)code << endl;
		
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
				doDataAnalyze();
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
		cout << "[ValiateServiceHandler][onShutdown]" << endl;
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
	unsigned short port = 5001; // maybe default port is 5001
	if (argc > 2)
		port = atoi(argv[2]);

	_dbhelper.init(dbPath);

	// set-up a server socket
	ServerSocket svs(port);
	// set-up a SocketReactor...
	SocketReactor reactor;
	// ... and a SocketAcceptor
	SocketAcceptor<ValiateServiceHandler> acceptor(svs, reactor);
	// run the reactor in its own thread so that we can wait for 
	// a termination request
	Thread thread;
	thread.start(reactor);

	cout << "ValiateServer started!, port: " << port << ", dbPath: " << dbPath << endl;

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

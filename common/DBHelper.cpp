#include "DBHelper.h"
#include "ComDef.h"


#include <iostream>

using namespace std;


bool checkUserfinger(const unsigned char *temp1, int len1, const unsigned char *temp2, int len2)
{
	// add your code here.
	
	return true;
}

bool DBHelper::findUser(const string& userId)
{
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("select count(*) from UserInfo where userid = %Q;", userId.c_str());
		return (_db.execScalar(bufSQL) != 0);
	}
	catch (CppSQLite3Exception& e)
	{
		cerr << e.errorCode() << ":" << e.errorMessage();
		return false;
	}
	return true;
}

int DBHelper::findAllUser(list<string>& passwdList)
{
	try
	{
		CppSQLite3Query q;
		CppSQLite3Binary blob;

		q = _db.execQuery("select userid from UserInfo;");
		bool find = false;

		while (!q.eof())
		{
			if (!find) find = true;
			passwdList.push_back(q.fieldValue("userid"));
			q.nextRow();
		}
		if (find)
		{
			cout << "findAllUser OK!";
			return success;
		}
		else
		{
			cout << "Not find any user!";
			return noUser;
		}
	}
	catch (CppSQLite3Exception& e)
	{
		cerr << e.errorCode() << ":" << e.errorMessage();
		return DBError;
	}
	return success;
}

int DBHelper::findUser(const string& userId, Message& msg)
{
	try
	{
		if (!findUser(userId))
			return noUser;

		CppSQLite3Buffer bufSQL;
		CppSQLite3Query q;
		CppSQLite3Binary blob;
		bufSQL.format("select * from UserInfo where userid = %Q;", userId.c_str());

		q = _db.execQuery(bufSQL);

		if (!q.eof())
		{
			msg.userId = q.fieldValue("userid");
			msg.userIdLen = msg.userId.length();
			msg.passwd = q.fieldValue("password");
			msg.passwdLen = msg.passwd.length();
			blob.setEncoded((unsigned char*) q.fieldValue("finger"));
			memset(msg.finger, 0, sizeof(msg.finger));
			memcpy(msg.finger, (unsigned char*) blob.getBinary(), blob.getBinaryLength());
			msg.fingerLen = blob.getBinaryLength();

			cout << "findUser " << userId << "OK!";
			return success;
		}
		cout << "not find " << userId;
		return noUser;
	}
	catch (CppSQLite3Exception& e)
	{
		cerr << e.errorCode() << ":" << e.errorMessage();
		return DBError;
	}
	return success;
}

int DBHelper::init(const string& dbPath)
{
	try
	{
		_db.open(dbPath.c_str());

		if (!_db.tableExists("UserInfo"))
		{
			_db.execDML("create table UserInfo(userid char(50), password char(399), finger blob);");
		}
	}
	catch (CppSQLite3Exception& e)
	{
		cerr << e.errorCode() << ":" << e.errorMessage() << endl;
		return DBError;
	}
	return success;
}

int DBHelper::addUser(const Message& msg)
{
	return addUser(msg.userId, msg.passwd, msg.finger, msg.fingerLen);
}

int DBHelper::addUser(const string& userId, const string& passwd, const unsigned char *finger, int fingerLen)
{
	try
	{
		if (findUser(userId))
			return userExists;

		CppSQLite3Binary blob;
		CppSQLite3Buffer bufSQL;
		blob.setBinary(finger, fingerLen);
		bufSQL.format("insert into UserInfo values (%Q, %Q, %Q);", userId.c_str(), passwd.c_str(), blob.getEncoded());
		_db.execDML(bufSQL);
/*
		CppSQLite3Query q;
		bufSQL.format("select finger from UserInfo where userid = %Q;", userId.c_str());
		q = _db.execQuery(bufSQL);

		if (!q.eof())
		{
			blob.setEncoded((unsigned char*)q.fieldValue("finger"));
			cout << "Retrieved binary Length: "
				<< blob.getBinaryLength() << endl;
		}*/
	}
	catch (CppSQLite3Exception& e)
	{
		cerr << e.errorCode() << ":" << e.errorMessage() << endl;
		return DBError;
	}
	return success;
}


int DBHelper::delUser(const Message& msg)
{
	return delUser(msg.userId);
}

int DBHelper::delUser(const string& userId)
{
	try
	{
		if (!findUser(userId))
			return noUser;

		CppSQLite3Buffer bufSQL;
		bufSQL.format("delete from UserInfo where userid = %Q;", userId.c_str());
		_db.execDML(bufSQL);
	}
	catch (CppSQLite3Exception& e)
	{
		cerr << e.errorCode() << ":" << e.errorMessage() << endl;
		return DBError;
	}
	return success;
}


int DBHelper::updateUser(const Message& msg)
{
	return updateUser(msg.userId, msg.passwd, msg.finger, msg.fingerLen);
}

int DBHelper::updateUser(const string& userId, const string& passwd, const unsigned char *finger, int fingerLen)
{
	try
	{
		if (!findUser(userId))
			return noUser;

		CppSQLite3Binary blob;
		CppSQLite3Buffer bufSQL;
		blob.setBinary(finger, fingerLen);
		bufSQL.format("update UserInfo set password = %Q, finger = %Q where userid = %Q;", passwd.c_str(), blob.getEncoded(), userId.c_str());
		_db.execDML(bufSQL);
	}
	catch (CppSQLite3Exception& e)
	{
		cerr << e.errorCode() << ":" << e.errorMessage() << endl;
		return DBError;
	}
	return success;
}



int DBHelper::validateUser(string& userId, string& passwd, const unsigned char *finger, int fingerLen)
{
	try
	{
		CppSQLite3Buffer bufSQL;
		CppSQLite3Query q = _db.execQuery("select * from UserInfo");
		bool find = false;
		const char *pUserId = NULL;
		const char *pPasswd = NULL;
		const unsigned char *pBlob = NULL;
		while (!q.eof())
		{

			pUserId = q.fieldValue(0);
			pPasswd = q.fieldValue(1);

			CppSQLite3Binary blob;
			blob.setEncoded((unsigned char*)q.fieldValue("finger"));
			cout << "Retrieved binary Length: "<< blob.getBinaryLength() << endl;

			const unsigned char* pbin = blob.getBinary();

			// checkUserfinger
			if (checkUserfinger(pbin, blob.getBinaryLength(), finger, fingerLen))
			{
				find = true;
				break;
			}
			q.nextRow();
		}

		if (find)
		{
			userId = pUserId;
			passwd = pPasswd;
			return success;
		}
		return notFind;
	}
	catch (CppSQLite3Exception& e)
	{
		cerr << e.errorCode() << ":" << e.errorMessage() << endl;
		return DBError;
	}
	return success;
}

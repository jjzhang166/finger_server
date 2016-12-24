#ifndef DBHelper_H
#define DBHelper_H

#include <string>
#include <vector>
#include "CppSQLite3.h"
#include "ComDef.h"
#include <list>

class DBHelper
{
public:

	int init(const std::string& dbPath);
	bool findUser(const std::string& userId);
	int findUser(const std::string& userId, Message& msg);
	int findAllUser(std::list<std::string>& passwdList);

	int addUser(const Message&);
	int addUser(const std::string& userId, const std::string& passwd, const unsigned char *finger, int fingerLen);


	int delUser(const Message&);
	int delUser(const std::string& userId);

	int updateUser(const Message&);
	int updateUser(const std::string& userId, const std::string& passwd, const unsigned char *finger, int fingerLen);

	int validateUser(std::string& userId, std::string& passwd, const unsigned char *finger, int fingerLen);
private:
	CppSQLite3DB _db;
private:
	bool findUser(const std::string& userId, const std::string& passwd, const unsigned char *finger, int fingerLen);
};

#endif

#ifndef COM_DEF_H
#define COM_DEF_H

#include <string>

#define FINGGER_BUF  4096

enum OptionCode
{
	queryUserInfo = 0,
	addUser = 1,
	delUser = 2,
	updateUser = 3,
	validateUser = 4,
	queryAllUserInfo = 5
};

enum ReturnCode
{
	success = 0,
	netError = 1,
	noUser = 2,
	userExists = 3,
	DBError = 4,
	notFind = 7,
	inputDataError = 8,
	passwdError = 255
};

struct Message
{
	unsigned char option;
	unsigned int userIdLen;
	std::string userId;
	unsigned int passwdLen;
	std::string passwd;
	unsigned int fingerLen;
	unsigned char finger[FINGGER_BUF];
};


#endif
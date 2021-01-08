#ifndef _LOG_MANAGER_H_
#define _LOG_MANAGER_H_

#include<iostream>
#include<string>
#include<time.h>
#include<fstream>

#define _CRT_SECURE_NO_WARNINGS
//#define INFO ""
//#define LOG (std::cout << config::getInstance()->getTimeNow() << " " << __FILE__ << " " << __func__ << " " << __LINE__ << ": ")

#define LOG (LOGManager() << /*getTimeNow() << " " << __FILE__ << " " <<*/ "thared id: " << std::this_thread::get_id() << " " <<  __func__ << " " << __LINE__ << ": ")

inline std::string getTimeNow()
{
	time_t now = time(0);
	tm* ltm = localtime(&now);
	char tmp[25] = { 0 };
	sprintf(tmp, "%d-%02d-%02d %02d:%02d:%02d", 1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
	return std::string(tmp);
}

class LOGManager {
public:
	LOGManager() { os = &std::cout; }
	~LOGManager() { std::cout << std::endl; }
	template<typename T>
	std::ostream& operator<< (T str)
	{
		//if (_DEBUG)
			return std::cout << str;
		//else
			//return ss;
	}

public:
	std::ostream* os;
	//std::stringstream ss;
};


#endif
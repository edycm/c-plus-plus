#ifndef _LOG_H_
#define _LOG_H_

#include <iostream>

#define LOGINFO(X)  std::cout << "INFO|" << __FILE__ << "::" << __func__ << "::" << __LINE__ << "| " << X << std::endl
#define LOGDEBUG(X) std::cout << "DEBUG|" << __FILE__ << "::" << __func__ << "::" << __LINE__ << "| " << X << std::endl
#define LOGWARN(X)  std::cout << "WARN|" << __FILE__ << "::" << __func__ << "::" << __LINE__ << "| " << X << std::endl
#define LOGERROR(X) std::cout << "ERROR|" << __FILE__ << "::" << __func__ << "::" << __LINE__ << "| " << X << std::endl


#endif
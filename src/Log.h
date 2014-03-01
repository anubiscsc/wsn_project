/*
 * Log.h
 *
 *  Created on: Feb 18, 2014
 *      Author: anubiscsc
 */

#ifndef LOG_H_
#define LOG_H_

#include <systemc>
using namespace sc_core;
using namespace std;

//Singleton Class
class Log {

public:

	static Log* Instance();
	void SC_log(std::ostringstream);
	std::ofstream& SC_log();

private:

	Log(const char*);
	static Log* m_pInstance;
	std::ofstream m_stream;

};

#endif /* LOG_H_ */

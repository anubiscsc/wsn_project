/*
 * Log.h
 *
 *  Created on: Feb 18, 2014
 *      Author: anubiscsc
 */

#ifndef LOG_H_
#define LOG_H_

#include <systemc>

//Singleton Class
class Log {

public:

	static Log* Instance_PHY();
	static Log* Instance_MAC();
	static Log* Instance_Stats();
	void SC_log(std::ostringstream);
	std::ofstream& SC_log();
	std::ofstream& get_ofstream();

private:

	Log(const char*);
	static Log* m_pInstance_PHY;
	static Log* m_pInstance_MAC;
	static Log* m_pInstance_Stats;
	std::ofstream m_stream;

};

#endif /* LOG_H_ */

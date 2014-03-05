/*
 * Log.cpp
 *
 *  Created on: Feb 18, 2014
 *      Author: anubiscsc
 */

#include "log.h"

using namespace sc_core;
using namespace std;

Log* Log::m_pInstance_PHY = 0;
Log* Log::m_pInstance_MAC = 0;
Log* Log::m_pInstance_Stats = 0;

Log* Log::Instance_PHY(){

	if(!Log::m_pInstance_PHY)
		Log::m_pInstance_PHY = new Log("PHY_log");

	return m_pInstance_PHY;
}

Log* Log::Instance_MAC(){

	if(!Log::m_pInstance_MAC)
		Log::m_pInstance_MAC = new Log("MAC_log");

	return m_pInstance_MAC;
}

Log* Log::Instance_Stats(){
	if(!Log::m_pInstance_Stats)
		Log::m_pInstance_Stats = new Log("STATS_log");

	return m_pInstance_Stats;
}

Log::Log(const char* filename) {
	// TODO Auto-generated constructor stub
	m_stream.open(filename);
}

void Log::SC_log(std::ostringstream msg)
{
	m_stream << "time " << sc_core::sc_time_stamp() << ": " << msg << std::endl;
}

// Method to be used inside C++ streams ( we can write: my_log->SC_log() << name() << ". My log message" << std::endl; )
std::ofstream& Log::SC_log(){

	m_stream << "time " << sc_time_stamp() << ": ";
	return m_stream;

}

ofstream& Log::get_ofstream(){
	return m_stream;
}

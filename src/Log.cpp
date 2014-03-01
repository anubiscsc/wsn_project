/*
 * Log.cpp
 *
 *  Created on: Feb 18, 2014
 *      Author: anubiscsc
 */

#include "Log.h"

Log* Log::m_pInstance = NULL;

Log* Log::Instance(){

	if(!Log::m_pInstance)
		Log::m_pInstance = new Log("log");

	return m_pInstance;
}

Log::Log(const char* filename) {
	// TODO Auto-generated constructor stub
	m_stream.open(filename);
}

void Log::SC_log(std::ostringstream msg)
{
	m_stream << "time " << sc_core::sc_time_stamp() << ": "<< msg << std::endl;
}

// Method to be used inside C++ streams // we can write: my_log->SC_log() << name() << ". My log message" << std::endl;

std::ofstream& Log::SC_log(){

	m_stream << "time " << sc_time_stamp() << ": ";
	return m_stream;

}

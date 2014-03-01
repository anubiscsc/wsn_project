/*
 * channel_if.h
 *
 *  Created on: Feb 26, 2014
 *      Author: anubiscsc
 */

#ifndef CHANNEL_IF_H_
#define CHANNEL_IF_H_

#include <systemc.h>

class channel_if : public sc_interface{
	public:
		virtual int num_packets() = 0;
		virtual int channel_packetRate() = 0;
};

#endif /* CHANNEL_IF_H_ */

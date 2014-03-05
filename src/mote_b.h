/*
 * mote_b.h
 *
 *  Created on: Jun 25, 2014
 *      Author: anubiscsc
 */

#ifndef MOTE_B_H_
#define MOTE_B_H_

#include "top.h"
#include "mote_base.h"

class Mote_b: public sc_module, public Mote_base {
public:
	SC_HAS_PROCESS(Mote_b);
	Mote_b(sc_module_name);
private:
	void recv_DATA_packet();
	void send_ACK_packet();

};

#endif /* MOTE_B_H_ */

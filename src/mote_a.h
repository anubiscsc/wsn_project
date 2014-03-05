/*
 * mote.h
 *
 *  Created on: Mar 24, 2014
 *      Author: anubiscsc
 */

#ifndef MOTE_H_
#define MOTE_H_

#include "top.h"
#include "mote_base.h"

class Mote_a: public sc_module, public Mote_base{
public:
	SC_HAS_PROCESS(Mote_a);
	Mote_a(sc_module_name, int);
	static void SetMACParams(sc_time, int);
	static sc_time get_backoff_time();

private:
	void mote_a_PHY_simulation();
	void mote_a_MAC1_simulation();
	void send_DATA_packet(int*);

	static sc_time backoff;
	static int max_tries;
	int num_tries; //number of tries that each mote has been commited
};

#endif /* MOTE_H_ */

/*
 * Top.h
 *
 *  Created on: Feb 26, 2014
 *      Author: anubiscsc
 */

#ifndef TOP_H_
#define TOP_H_

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/multi_passthrough_initiator_socket.h"
#include "tlm_utils/multi_passthrough_target_socket.h"
#include "tlm_utils/peq_with_cb_and_phase.h"
#include <queue>

//#define DEBUG

class mote;
class channel;

SC_MODULE(Top){

	channel* channel1;
	SC_HAS_PROCESS(Top);
	Top(sc_core::sc_module_name, int);

};

#endif /* TOP_H_ */

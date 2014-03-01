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

#include "Log.h"
//#include "state_extension.h"
//#include "collision_extension.h"
//#define DEBUG

class state_extension;
class collision_extension;
class Mote;
class Channel;

SC_MODULE(Top){

	Channel* channel1;
	SC_HAS_PROCESS(Top);
	Top(sc_core::sc_module_name, int);

};

#endif /* TOP_H_ */

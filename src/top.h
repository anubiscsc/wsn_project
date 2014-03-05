/*
 * Top.h
 *
 *  Created on: Feb 26, 2014
 *      Author: anubiscsc
 */

#ifndef TOP_H_
#define TOP_H_

/* DEFINES */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#define LOG_PHY_FLAG 0
#define LOG_MAC_FLAG 0
#define MAC_ENABLE 1
#define MAX_ITERATIONS 10
#define BACKOFF_TIME_MULT 3
//#define DEBUG

#include <systemc.h>
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/multi_passthrough_initiator_socket.h"
#include "tlm_utils/multi_passthrough_target_socket.h"
#include "tlm_utils/peq_with_cb_and_phase.h"
#include <queue>

extern sc_time get_delay(const int&, const int&);

class Mote;
class Channel;
class Log;
class Stats;

extern double simulation_time;
extern std::vector<int> channel_bitrate_v;
extern sc_time_unit TIME_UNITS;
extern int MAX_BITRATE;
extern double MAX_IDLE_TIME;
extern int NUM_CHANNELS;
extern int NUM_MOTES;
extern int BASE_MOTE_ID;
extern int active_motes;
extern sc_event halt_event;
extern std::random_device rd;
extern std::default_random_engine e0;
extern std::mt19937_64 e1;


SC_MODULE(Top){

	Channel* channel1;
	SC_HAS_PROCESS(Top);
	Top(sc_core::sc_module_name);

};

#endif /* TOP_H_ */

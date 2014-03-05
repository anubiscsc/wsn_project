/*
 * mote_b.cpp
 *
 *  Created on: Jun 25, 2014
 *      Author: anubiscsc
 */

#include "mote_b.h"
#include "log.h"

using namespace sc_core;
using namespace std;

Mote_b::Mote_b(sc_module_name _name) :
		sc_module(_name),
		Mote_base(_name, -1)
{

#if LOG_MAC_FLAG
	log_mac->SC_log() << get_name() << ": STARTING base_mote_MAC1_simulation | mote_id=" << mote_id  << endl;
#endif
	SC_THREAD(recv_DATA_packet);

}

//base mote keeps listening (through all channels)
void Mote_b::recv_DATA_packet(){

	int buff, channelId;
	std::uniform_int_distribution<> distr(0, NUM_CHANNELS-1);

	while(1){
		buff = 0;
		channelId = distr(e0);
		recv(channelId, buff, sizeof(buff)); //keeps waiting until one transaction is received
		if(active_motes == 1){
			break;
		}
	}

#ifdef DEBUG
	cout << get_name() << ": checkpoint finish!" << endl;
#endif
#if LOG_PHY_FLAG
	log->SC_log() << get_name() << ":" << endl;
	log->SC_log() << get_name() << ": <<< END OF SIMULATION >>>" << endl;
	log->SC_log() << get_name() << ":" << endl;
#endif
}

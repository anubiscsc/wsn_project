/*
 * channel.h
 *
 *  Created on: 03/02/2014
 *      Author: Anubiscsc
 */

#ifndef CHANNEL_H_
#define CHANNEL_H_

#include "Top.h"
#include "Log.h"

class channel: sc_module{
private:
	Log* log;
	int npackets;
	sc_time colision_time;
	sc_event end_collisions_event;
	//int lostPacketRate;
	std::map <tlm::tlm_generic_payload*, unsigned int> m_id_map;
	std::map <mote::state_t, int> state_id_map;

	tlm::tlm_sync_enum nb_transport_bw(int, tlm::tlm_generic_payload&, tlm::tlm_phase&, sc_time&);
	tlm::tlm_sync_enum nb_transport_fw(int, tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_time& delay);
	//unsigned int transport_dbg(int, tlm::tlm_generic_payload&);
	void set_mote_state(int, mote::state_t);
	mote::state_t get_mote_state(int);

public:

	tlm_utils::multi_passthrough_initiator_socket<channel> mpi_socket;
	tlm_utils::multi_passthrough_target_socket<channel> mpt_socket;

	SC_HAS_PROCESS(channel);
	channel(sc_core::sc_module_name);

};


#endif /* CHANNEL_H_ */

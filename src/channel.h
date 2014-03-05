/*
 * channel.h
 *
 *  Created on: 03/02/2014
 *      Author: Anubiscsc
 */

#ifndef CHANNEL_H_
#define CHANNEL_H_

#include "top.h"
#include "mote_base.h"

class state_extension;
class collision_extension;
class channel_extension;

class Channel: sc_module{

private:
	Log* log;
	Stats* stats;
	int npackets;
	bool new_trans;
	sc_event channel_control_event; //lock the main channel control loop until a transaction begins to be transmitted or is going to be released

	std::map <tlm::tlm_generic_payload*, int> m_id_map;
	std::map <sc_time, tlm::tlm_generic_payload*> time_m_map;
	std::map <unsigned int, Mote_base::state_t> state_id_map;

	tlm::tlm_sync_enum nb_transport_bw(int, tlm::tlm_generic_payload&, tlm::tlm_phase&, sc_time&);
	tlm::tlm_sync_enum nb_transport_fw(int, tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_time& delay);

	inline void set_collision(tlm::tlm_generic_payload&, collision_extension&);
	void set_mote_state(int, Mote_base::state_t);
	Mote_base::state_t get_mote_state(int);
	int count_mote_states(int, Mote_base::state_t);
	int count_mote_channels(const int&);
	void channel_control();

public:
	tlm_utils::multi_passthrough_initiator_socket<Channel> mpi_socket;
	tlm_utils::multi_passthrough_target_socket<Channel> mpt_socket;

	SC_HAS_PROCESS(Channel);
	Channel(sc_core::sc_module_name);

};

#endif /* CHANNEL_H_ */

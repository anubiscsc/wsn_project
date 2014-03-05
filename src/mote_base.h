/*
 * mote.h
 *
 *  Created on: 03/02/2014
 *      Author: Anubiscsc
 */

#ifndef MOTE_BASE_H_
#define MOTE_BASE_H_

#include "top.h"
#include "mm.h"
class state_extension;
class collision_extension;

class mm;

class Mote_base{

public:

	tlm_utils::simple_initiator_socket<Mote_base> i_socket;
	tlm_utils::simple_target_socket<Mote_base> t_socket;
	int mote_id;
	int listening_channel_id;
	bool response_in_progress;
	tlm::tlm_generic_payload* request_in_progress;
	tlm::tlm_generic_payload* current_transaction;
	tlm_utils::peq_with_cb_and_phase<Mote_base> m_peq;
	tlm_utils::peq_with_cb_and_phase<Mote_base> send_ack_peq;

	string& get_name();
	Mote_base(sc_module_name, int);
	enum state_t {IDLE, LISTENING, SENDING}; //mote state options


protected:
	//struct position{int x; int y; int z; }; //class which provides distance and position calculation
	//position pos;

	string module_name;
	state_t prev_state;
	int* buffer;

	mm m_mm;
	Log* log;
	Log* log_mac;
	Stats* stats;

	sc_event end_request_event; //phase: END_REQ. Not used.
	sc_event trans_recv_event;
	sc_event ack_recv_event;

	void update_channel(state_t);
	tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_time& delay);
	tlm::tlm_sync_enum nb_transport_bw( tlm::tlm_generic_payload&, tlm::tlm_phase&, sc_time&);

	void peq_cb(tlm::tlm_generic_payload&, const tlm::tlm_phase&);
	void send_response(tlm::tlm_generic_payload&, const tlm::tlm_phase&);
	void check_transaction(tlm::tlm_generic_payload&);
	tlm::tlm_sync_enum send_end_req(tlm::tlm_generic_payload&);
	void recv(int, int&, int);
	void send(int, int*, int);
};

ostream& operator<<(ostream& out, const Mote_base::state_t& state);

#endif /* MOTE_BASE_H_ */

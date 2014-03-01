/*
 * mote.h
 *
 *  Created on: 03/02/2014
 *      Author: Anubiscsc
 */

#ifndef MOTE_H_
#define MOTE_H_

#include "Top.h"
#include "mm.h"

class Log;

class Mote : sc_module{

private:
	struct position{int x; int y; int z; }; //class which provides distance and position calculation
	position pos;


	int buffer[10];
	mm m_mm;
	Log* log;
	tlm::tlm_sync_enum nb_transport_bw( tlm::tlm_generic_payload&, tlm::tlm_phase&, sc_time&);
	tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_time& delay);
	//unsigned int transport_dbg(int, tlm::tlm_generic_payload&);
	void peq_cb(tlm::tlm_generic_payload&, const tlm::tlm_phase&);
	void check_transaction(tlm::tlm_generic_payload&);
	void thread_process();
	tlm::tlm_sync_enum send_end_req(tlm::tlm_generic_payload&);
	void send_response(tlm::tlm_generic_payload&);

public:
	enum state_t {LISTENING_CHANNEL, STANDBY, SENDING_TRANS}; //mote state options
	//ostream& operator<<(ostream&, state_t);

	tlm_utils::simple_initiator_socket<Mote> i_socket;
	tlm_utils::simple_target_socket<Mote> t_socket;

	//state_t m_state;
	bool  response_in_progress;
	tlm::tlm_generic_payload* request_in_progress;
	sc_event end_request_event;
	tlm_utils::peq_with_cb_and_phase<Mote> m_peq;

	void send_data(void);
	void recv_data(void);
	void listen_channel(void);

	SC_HAS_PROCESS(Mote);
	Mote(sc_module_name);
};

ostream& operator<<(ostream& out, const Mote::state_t& state);

#endif /* MOTE_H_ */

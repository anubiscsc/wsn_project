/*
 * channel.cpp
 *
 *  Created on: 03/02/2014
 *      Author: Anubiscsc
 */

#include "channel.h"

using namespace sc_core;
using namespace sc_dt;
using namespace std;

channel::channel(sc_module_name name_) :
		sc_module(name_),
		npackets(0),
		mpi_socket("mpi_socket"),
		mpt_socket("mpt_socket")
{
	log = Log::Instance();
	mpi_socket.register_nb_transport_bw(this, &channel::nb_transport_bw);
	mpt_socket.register_nb_transport_fw(this, &channel::nb_transport_fw);
	//mpt_socket.register_transport_dbg(this, &channel::transport_dbg);

}

// Tagged non-blocking transport forward method --- FORWARD PATH
tlm::tlm_sync_enum channel::nb_transport_fw(int id, tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_time& delay)
{
#ifdef DEBUG
	assert(id < mpt_socket.size());
#endif

	tlm::tlm_sync_enum status;

	set_mote_state(id, mote::SENDING_TRANS); //update the state of the mote sending the transaction. New state= mote::SENDING_TRANS

	m_id_map[ &trans ] = id;
	int n_bounds = static_cast<int> (mpt_socket.size());

	//log->SC_log() << "Sending Transaction through "<< name() << " to " << n_sockets-1 << " motes" << endl;
	log->SC_log() << "IC " << name() << " <----> Broadcasting Transaction to " << n_bounds-1 << " motes..." << endl;
	for (int i = 0; i < n_bounds; i++){
	  // Forward transaction to all targets (broadcast)
		if(i != id){
			delay = sc_time(rand()%1000, SC_NS);
			status = mpi_socket[i]->nb_transport_fw(trans, phase, delay);
		}
	}

	log->SC_log() << endl;
	return status;
}

// non-blocking transport backward method
tlm::tlm_sync_enum channel::nb_transport_bw(int id, tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_time& delay){

	#ifdef DEBUG
		assert (id < mpi_socket.size());
	#endif

	// Backward path
	return mpt_socket[ m_id_map[ &trans ] ]->nb_transport_bw(trans, phase, delay);

}

void channel::set_mote_state(int mote_id, mote::state_t state){ state_id_map[mote_id] = state; }

mote::state_t channel::get_mote_state(int id){ return state_id_map[id]; }

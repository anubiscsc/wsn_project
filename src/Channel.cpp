/*
 * channel.cpp
 *
 *  Created on: 03/02/2014
 *      Author: Anubiscsc
 */

#include "Channel.h"
#include "state_extension.h"
#include "collision_extension.h"

using namespace sc_core;
using namespace sc_dt;
using namespace std;

Channel::Channel(sc_module_name name_) :
		sc_module(name_),
		npackets(0),
		mpi_socket("mpi_socket"),
		mpt_socket("mpt_socket")
{
	log = Log::Instance();
	mpi_socket.register_nb_transport_bw(this, &Channel::nb_transport_bw);
	mpt_socket.register_nb_transport_fw(this, &Channel::nb_transport_fw);
	//mpt_socket.register_transport_dbg(this, &channel::transport_dbg);

}

// Tagged non-blocking transport forward method --- FORWARD PATH
tlm::tlm_sync_enum Channel::nb_transport_fw(int id, tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_time& delay)
{
#ifdef DEBUG
	assert(id < mpt_socket.size());
#endif

	tlm::tlm_sync_enum status;
	Mote::state_t new_state;
	state_extension* s_ext;

	trans.get_extension(s_ext); //s_ext gets points to the generic payload extension address
	if(s_ext->state_change){ //checks whether the mote state has changed or not
		new_state = s_ext->state;
		set_mote_state(id, new_state); //update the state of the mote that is communicating with channel.
		log->SC_log() << name() << ": state of mote_" << id << " has changed. New state=" << s_ext->state << endl;
	}

	if(s_ext->state == Mote::SENDING_TRANS){
		m_id_map[ &trans ] = id;
		int n_bounds = static_cast<int> (mpt_socket.size());

		//modify/corrupt data attibute if there is any mote in a SENDING_TRANS state and sets collision extension to true
		unsigned int count = count_motes(id, Mote::SENDING_TRANS);
		cout << sc_time_stamp() << ": " << count << " motes sending through channel" << endl;

		if(count > 0){
			log->SC_log() << endl;

			collision_extension* c_ext;
			trans.get_extension(c_ext);
			c_ext->is_Collision = true; //tells to the target mote that the transaction is corrupted because there have been a collision in the channel
			int* data = reinterpret_cast<int*>( trans.get_data_ptr() );
			trans.set_data_ptr(reinterpret_cast<unsigned char*>( (*data | 0xAA))); //corrupts data to demostrate the transaction collision
			cout << "Channel checkpoint: if collision " << endl;

		}

		//log->SC_log() << "Sending Transaction through "<< name() << " to " << n_sockets-1 << " motes" << endl;
		log->SC_log() << name() << " <----> Broadcasting Transaction from mote_" << id << " to network (" << n_bounds-1 << " destination motes)" << endl;
		for (int i = 0; i < n_bounds; i++){ // Forward transaction to all targets (broadcast),
			if(i != id){ //excluding the emitting mote itself,
				if(get_mote_state(i) != Mote::STANDBY){ // and only to non-STANDBY motes
					log->SC_log() << name() << " <----> Sending Transaction from mote_" << id << " to mote_" << i << " | Address=" << hex << trans.get_address() << endl;
					delay = sc_time(rand()%1000, SC_US);
					status = mpi_socket[i]->nb_transport_fw(trans, phase, delay);
				}
			}
		}
		//log->SC_log() << endl;
	}
	//cout << "checkpoint0 channel" << endl;
	trans.set_response_status(tlm::TLM_OK_RESPONSE); //tells to the initiator mote that the status of transaction response is OK
	status = tlm::TLM_COMPLETED; //force status to TLM_COMPLETED since we are not following the full base protocol

	return status;
}

// non-blocking transport backward method
tlm::tlm_sync_enum Channel::nb_transport_bw(int id, tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_time& delay){

	#ifdef DEBUG
		assert (id < mpi_socket.size());
	#endif

	// Backward path
	return mpt_socket[ m_id_map[ &trans ] ]->nb_transport_bw(trans, phase, delay);

}

void Channel::set_mote_state(int mote_id, Mote::state_t state){ state_id_map[mote_id] = state; }

Mote::state_t Channel::get_mote_state(int id){ return state_id_map[id]; }

//Counts how many motes are in the state specified by parameter
unsigned int Channel::count_motes(int id, Mote::state_t state){
	unsigned int cnt;
	typedef map<unsigned int, Mote::state_t>::iterator it_t;
	cnt = 0;
	for(it_t iter = state_id_map.begin(); iter != state_id_map.end(); iter++){
		if(iter->second == state && iter->first != static_cast<unsigned int>(id)){
			cnt++;
		}
	}
	return cnt;
}

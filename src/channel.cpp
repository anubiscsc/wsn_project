/*
 * channel.cpp
 *
 *  Created on: 03/02/2014
 *      Author: Anubiscsc
 */

#include "channel.h"
#include "state_extension.h"
#include "collision_extension.h"
#include "channel_extension.h"
#include "log.h"
#include "stats.h"

using namespace sc_core;
using namespace sc_dt;
using namespace std;

Channel::Channel(sc_module_name name_) :
		sc_module(name_),
		npackets(0),
		new_trans(0),
		mpi_socket("mpi_socket"),
		mpt_socket("mpt_socket")
		//sending_uniform_dist(1, MAX_SENDING_TIME)
{
#if LOG_PHY_FLAG
	log = Log::Instance_PHY();
#else
	log = 0;
#endif
	stats = Stats::Instance();
	channel_bitrate_v.resize(NUM_CHANNELS);

	for(unsigned int i = 0; i < channel_bitrate_v.size(); i++){
		channel_bitrate_v[i] = (MAX_BITRATE*0.9) + (i*( (MAX_BITRATE*0.1)/NUM_CHANNELS ) );
		//cout << "channel_bitrate_v[" << i << "]=" << channel_bitrate_v[i] << endl;
	}
	mpt_socket.register_nb_transport_fw(this, &Channel::nb_transport_fw);
	if(MAC_ENABLE)
		mpi_socket.register_nb_transport_bw(this, &Channel::nb_transport_bw);

	SC_THREAD(channel_control);
}

void Channel::channel_control(){

	channel_extension* ch_ext;
	collision_extension* c_ext;
	sc_time next_delay, prev_timestamp;
	const sc_time halt_delay = sc_time(MAX_IDLE_TIME, TIME_UNITS);

	while(1){
		if(time_m_map.size() == 0){
			//initialize the next delay in order to ensure that when there's no transaction in channel,
			//channel_control_event will trigger the thread execution
			next_delay = halt_delay;
		}
		prev_timestamp = sc_time_stamp();
		wait(next_delay, channel_control_event | halt_event); //waits till the next control event (release of transaction or new transaction in channel)

		if((MAC_ENABLE && active_motes < 2) || (!MAC_ENABLE && active_motes == 1)){
#if LOG_PHY_FLAG
			log->SC_log() << name() << ".control: HALT" << endl;
#endif
//			stats->printStats(); //print the main simulation statistics on the default output
//			stats->log_Stats(); //print the main simulation statistics on the STATS_log file
			break; //end of channel control thread
		}

		//get next delay and erase the first entry of time_m_map
		if (time_m_map.size() > 0){
			do{
				if( sc_time_stamp() >= time_m_map.begin()->first ){
					m_id_map.erase(time_m_map.begin()->second); //erase all the map entries that match with the transaction address to be deleted from channel
					time_m_map.erase(time_m_map.begin()->first); //erase all the map entries that match with the first entry time simulation of time_m_map
				}
				if(time_m_map.size() == 0)
					break;

				next_delay = time_m_map.begin()->first - sc_time_stamp(); //calculate the next waiting sc_time
#if LOG_PHY_FLAG
				log->SC_log() << name() << ".control: next checking time=" << sc_time_stamp()+next_delay << endl;
#endif
			}while(time_m_map.begin()->first < sc_time_stamp());
		}

		if(time_m_map.size() > 1 && new_trans){
			//Checks if the transaction to be deleted has not collided, if hasn't, set collision status to true
			for(map<sc_time, tlm::tlm_generic_payload*>::iterator it = time_m_map.begin(); it!= time_m_map.end();++it){
				it->second->get_extension(c_ext);
				if(!c_ext->is_Collision){
					it->second->get_extension(ch_ext);
					// if there are at least 2 active transaction in the same channel scope, set the collision status to true
					if(count_mote_channels(ch_ext->id_channel) > 1){ //check if a transaction channel_id get a collision
#if LOG_PHY_FLAG
						log->SC_log() << name() << ".control: Collision Detected!! Address=" << hex << it->second->get_address() << dec
								<< " in channel " << ch_ext->id_channel << endl;
#endif
						stats->num_trans_lost += 1; //counter for simulation stats
						stats->ch_stats[ch_ext->id_channel].num_trans_lost +=1; //counter for simulation stats
						set_collision(*(it->second), *c_ext); //set a corrupted data atribute value to the transaction
					}
				}
			} //end_for
		} //end_if
		new_trans = false;
#if LOG_PHY_FLAG
		log->SC_log() << name() << ".control: " << time_m_map.size() << " transaction(s) being transmitted through " << name() << endl;
#endif
	} //end_while

}

// Tagged non-blocking transport forward method --- FORWARD PATH
tlm::tlm_sync_enum Channel::nb_transport_fw(int id, tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_time& delay)
{
#ifdef DEBUG
	assert((unsigned int)id < mpi_socket.size());
#endif

	tlm::tlm_sync_enum status;
	Mote_base::state_t new_state;
	state_extension* s_ext;

	trans.get_extension(s_ext); //s_ext gets points to the generic payload extension address
	if(s_ext->has_changed){ //checks whether the mote state has changed or not
		new_state = s_ext->state;
		set_mote_state(id, new_state); //update the state of the mote that is communicating with channel.
#if LOG_PHY_FLAG
		log->SC_log() << name() << ": state of mote_" << id << " has changed. New state=" << s_ext->state << endl;
#endif
	}
#if LOG_PHY_FLAG
	else
		log->SC_log() << name() << ": state of mote_" << id << " has not changed. State=" << s_ext->state << endl;
#endif
	if(s_ext->state == Mote_base::SENDING){
		channel_extension* ch_ext;
		collision_extension* c_ext = new collision_extension(); //Instanciate a new extension object. When called extension constructor, is_collision=false by default
		trans.set_auto_extension(c_ext);
		trans.get_extension(ch_ext);

		int n_bounds = static_cast<int> (mpi_socket.size());
		int count = 0; //count_mote_states(id, Mote::SENDING_TRANS);

#if LOG_PHY_FLAG
		log->SC_log() << name() << " " << ch_ext->id_channel << ": ----> Sending Transaction from mote_" << id
									<< " | Address=" << hex << trans.get_address()
									<< " | DATA=" << *reinterpret_cast<int*>( trans.get_data_ptr() ) << dec << endl;
#endif

		delay = get_delay(ch_ext->id_channel, trans.get_data_length());
		stats->num_trans_sent += 1; //increments the number of transactions for statistics purpose
		stats->ch_stats[ch_ext->id_channel].num_trans_sent += 1;
		for (int i = 0; i < n_bounds; i++){ // Forward transaction to all targets (broadcast),
			if(i != id){ //excluding the emitting mote itself,
				if(MAC_ENABLE){
#if LOG_PHY_FLAG
					log->SC_log() << name() << " " << ch_ext->id_channel << ": ----> Sending Transaction from mote_" << id << " to mote_" << i
							<< " | Address=" << hex << trans.get_address()
							<< " | DATA=" << *reinterpret_cast<int*>( trans.get_data_ptr() ) << dec << endl;
#endif
					status = mpi_socket[i]->nb_transport_fw(trans, phase, delay);
					count = 1; //avoid the next condition outside the for loop
				}
			}
		}

		stats->acum_time_trans += delay; //simulation stats acumulator
		if (count == 0) //Checking amount of motes with state=LISTENING_CHANNEL
			s_ext->force_change = true; //if there are no LISTENING_CHANNEL motes, tell the source mote to force the state change

		time_m_map[sc_time_stamp() + delay] = &trans; //add a new transaction with its sc_time value to be erased from time_m_map
		new_trans = true; //note that the execution release of channel control is active because of a new transaction in channel
		channel_control_event.notify(); //triggers the channel_control execution for updating the next_delay value
		m_id_map[ &trans ] = id;
	}

	trans.set_response_status(tlm::TLM_OK_RESPONSE); //tells to the initiator mote that the status of transaction response is OK
	if(MAC_ENABLE)
		status = tlm::TLM_ACCEPTED; //TLM_ACCEPTED indicates that there will be more phases to check in the base protocol
	else
		status = tlm::TLM_COMPLETED; //force status to TLM_COMPLETED since we are not following the full base protocol

	return status;
}

// non-blocking transport backward method
tlm::tlm_sync_enum Channel::nb_transport_bw(int id, tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_time& delay){

#ifdef DEBUG
	assert ((unsigned int)id < mpt_socket.size());
#endif
	int n_bounds = static_cast<int> (mpt_socket.size());
	tlm::tlm_sync_enum status;
	channel_extension* ch_ext;
	collision_extension* c_ext = new collision_extension(); //Instanciate a new extension object. When called extension constructor, is_collision=false by default
	trans.set_auto_extension(c_ext);
	trans.get_extension(ch_ext);


#if LOG_PHY_FLAG
	log->SC_log() << name() << " " << ch_ext->id_channel << ": ----> Sending back Transaction from base_mote"
			<< " | Address=" << hex << trans.get_address() //<< dec << endl;
			<< " | DATA=" << string(trans.get_data_ptr(), trans.get_data_ptr() + trans.get_data_length() ) << dec << endl;
#endif
	stats->num_trans_sent += 1; //increments the number of transactions for statistics purpose
	stats->ch_stats[ch_ext->id_channel].num_trans_sent += 1;
	delay = get_delay(ch_ext->id_channel, trans.get_data_length());

	for (int i = 0; i < n_bounds; i++){ // Forward transaction to all targets (broadcast),
		if(i != id){ //excluding the emitting mote itself,
			trans.acquire();
			/* Backward path */
			status = mpt_socket[i]->nb_transport_bw(trans, phase, delay);
		}
	}

	stats->acum_time_trans += delay; //simulation stats acumulator
	time_m_map[sc_time_stamp() + delay] = &trans; //add a new transaction with its sc_time value to be erased from time_m_map
	new_trans = true; //note that the execution release of channel control is active because of a new transaction in channel
	channel_control_event.notify(); //triggers the channel_control execution for updating the next_delay value
	trans.set_response_status(tlm::TLM_OK_RESPONSE); //tells to the initiator mote that the status of transaction response is OK
	status = tlm::TLM_COMPLETED;

	return status;
}

// set the state to the mote with id=mote_id to the transaction state pool of the channel module
void Channel::set_mote_state(int mote_id, Mote_base::state_t state){
	state_id_map[mote_id] = state;
}

// returns the state of the desired mote id
Mote_base::state_t Channel::get_mote_state(int id){
	return state_id_map[id];
}

//Counts how many motes are in the state specified by parameter
int Channel::count_mote_states(int id, Mote_base::state_t state){
	int c = 0;
	typedef map<unsigned int, Mote_base::state_t>::iterator it_t;
	for(it_t iter = state_id_map.begin(); iter != state_id_map.end(); iter++){
		if(iter->second == state && iter->first != static_cast<unsigned int>(id)){
			c++;
		}
	}
	return c;
}

int Channel::count_mote_channels(const int& id){
	int c = 0;
	channel_extension* ext2;

	typedef map<sc_time, tlm::tlm_generic_payload*>::iterator it_t;
	for(it_t iter = time_m_map.begin(); iter != time_m_map.end(); iter++){
		iter->second->get_extension(ext2);
		if(id == ext2->id_channel){
			c++;
		}
	}
	return c;
}

//modify/corrupt data attribute if there is any mote in a SENDING_TRANS state and sets collision extension to true
inline void Channel::set_collision(tlm::tlm_generic_payload& trans, collision_extension& c_ext){

	int* data;
	c_ext.is_Collision = true; //tells to the target mote that the transaction is corrupted because there have been a collision in the channel
	data = reinterpret_cast<int*>( trans.get_data_ptr() );
	*data |= 0xffffffff; //sets corrupted data (we consider corrupted data = 0xffffffff)
	trans.set_data_ptr( reinterpret_cast<unsigned char*>(data) ); //corrupts data to show the transaction collision

}

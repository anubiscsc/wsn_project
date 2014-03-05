/*
 * mote.cpp
 *
 *  Created on: Mar 24, 2014
 *      Author: anubiscsc
 */

#include "mote_a.h"
#include "state_extension.h"
#include "collision_extension.h"
#include "channel_extension.h"
#include "log.h"
#include "stats.h"

using namespace sc_core;
using namespace std;

sc_time Mote_a::backoff = SC_ZERO_TIME;
int Mote_a::max_tries = 0;

Mote_a::Mote_a(sc_module_name _name, int id) :
		sc_module(_name),
		Mote_base(_name, id),
		num_tries(0)
{

	if(MAC_ENABLE){
#if LOG_MAC_FLAG
		log_mac->SC_log() << get_name() << ": STARTING mote_MAC1_simulation | mote_id=" << mote_id << endl;
#endif
		SC_THREAD(mote_a_MAC1_simulation);
	}
#ifdef DEBUG
	else{
		cout << get_name() << ": STARTING mote_PHY_simulation | mote_id=" << mote_id << endl;
		SC_THREAD(mote_a_PHY_simulation);
	}
#endif
}

void Mote_a::SetMACParams(sc_time backoff_time, int tries){
	backoff = sc_time(backoff_time.to_double(), TIME_UNITS);
	max_tries = tries;
}

sc_time Mote_a::get_backoff_time(){
	return backoff;
}

//Simulation thread of MAC layer comunication (Mac1 protocol) for every mote
void Mote_a::mote_a_MAC1_simulation(){

	std::normal_distribution<> normal_distr(MAX_IDLE_TIME/2, 2);
	std::uniform_real_distribution<> uniform_distr(0, MAX_IDLE_TIME);
	int buff;
	int* data_ptr = &buff;

	wait(sc_time(uniform_distr(e0), TIME_UNITS));

	for (int i = 0; i < MAX_ITERATIONS; i++){ //repeat the process i times

		buff = uniform_distr(e0);
		stats->num_data_pck_created++;
		send_DATA_packet(data_ptr);
		current_transaction = 0; //reset the current transaction pointer in order to be used again
		wait(sc_time(normal_distr(e1), TIME_UNITS)); //each iteration, the mote waits a random time, generated by a Poisson distribution, to create asynchron mote sending behaviour
	}

#if LOG_PHY_FLAG
	log->SC_log() << name() << ":" << endl;
	log->SC_log() << name() << ": <<< END OF SIMULATION >>>" << endl;
	log->SC_log() << name() << ":" << endl;
#endif
	active_motes--;
	if(active_motes == 1){ halt_event.notify(); }
}


/* Simulation thread of Physical layer comunication for every mote (DEPRECATED) */
void Mote_a::mote_a_PHY_simulation(){

	int buff;
	int* data;
	std::uniform_int_distribution<> uniform_distr(1, MAX_IDLE_TIME);
	std::uniform_int_distribution<> uniform_distr_data(0, RAND_MAX);
	// -------- Update the channel state_id_map variable ------------------
	update_channel(prev_state); //at the start, the state is equal to IDLE
	// --------------------------------------------------------------------
	wait(sc_time(uniform_distr(e0), TIME_UNITS)); //each iteration, the mote waits a random time to create asynchron mote behaviour
	for (int i = 0; i < MAX_ITERATIONS; i++){ //repeat the process i times
		data = &buff;
		*data = uniform_distr_data(e0);
		switch(static_cast<Mote_base::state_t>(uniform_distr(e0) % 3)){
			case Mote_base::SENDING:
				send(uniform_distr(e0) % NUM_CHANNELS, data, sizeof(data)); //updates the mote state and sends a transaction
				break;
			case Mote_base::LISTENING:
				update_channel(Mote_base::LISTENING);
				recv(uniform_distr(e0) % NUM_CHANNELS, buff, sizeof(buff)); //keeps waiting until one transaction is recieved
				update_channel(Mote_base::IDLE);
				wait(sc_time(uniform_distr(e0), TIME_UNITS));
				break;
			case Mote_base::IDLE:
				update_channel(Mote_base::IDLE);
				wait(sc_time(uniform_distr(e0), TIME_UNITS));
				break;
			default:
				break;
		}
	}

	// -------- Update the channel state_id_map variable ------------------
	// force the mote state change to IDLE for finishing the mote simulation
	if (prev_state != Mote_base::IDLE)
		update_channel(Mote_base::IDLE);

	active_motes--;
	if(active_motes == 0){ halt_event.notify(); }

	cout << sc_module::name() << ": checkpoint finish!" << endl;
#if LOG_PHY_FLAG
	log->SC_log() << name() << ":" << endl;
	log->SC_log() << name() << ": <<< END OF SIMULATION >>>" << endl;
	log->SC_log() << name() << ":" << endl;
#endif

}

//send a data packet to a mote and waits until the receiving mote sends back an ACK packet.
void Mote_a::send_DATA_packet(int* data){

	sc_time delay, init_time;
	delay = SC_ZERO_TIME;
	int data_backup = *data;
	std::uniform_int_distribution<int> channel_uniform_dist(0, NUM_CHANNELS-1);

	do{
		if(num_tries > 0)
			*data = data_backup;
#if LOG_MAC_FLAG
		if(delay >= backoff){
			log_mac->SC_log() << "Initiator " << get_name() << " ----> Resending Packet | Phase=BEGIN_REQ | Try=" << num_tries << " | data=" << hex << *data << dec << endl;
		}
		else
			log_mac->SC_log() << "Initiator " << get_name() << " ----> Sending Packet | Phase=BEGIN_REQ | Try=" << num_tries << " | data=" << hex << *data << dec << endl;
#endif
		listening_channel_id = channel_uniform_dist(e1);
		send(listening_channel_id, data, sizeof(*data)); //updates the mote state and sends a transaction

		init_time = sc_time_stamp();
		wait(backoff, ack_recv_event);
		delay = sc_time_stamp() - init_time;
		num_tries++;
		//set a semaphor to encapsulate this mutual exclusion zone
		if(stats->mutex.trylock() != -1){
			stats->num_tries++;
			stats->acum_pck_delay += delay;
			stats->mutex.unlock();
		}
	}while((delay >= backoff) && (num_tries < max_tries));

#if LOG_MAC_FLAG
	if(num_tries == max_tries)
		log_mac->SC_log() << "Initiator " << get_name() << ": << Transaction sending failed after 5 tries! >>" << endl;
#endif
	num_tries = 0;
}

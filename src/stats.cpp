/*
 * Stats.cpp
 *
 *  Created on: Mar 31, 2014
 *      Author: anubiscsc
 */
#include <stddef.h>
#include "stats.h"
#include "log.h"
#include "top.h"
#include "mote_a.h"

using namespace std;

// Global static pointer used to ensure a single instance of the class.
Stats* Stats::m_pInstance = NULL;

Stats* Stats::Instance(){ // Only allows one instance to be generated.
	if(!Stats::m_pInstance)
		Stats::m_pInstance = new Stats();
	return m_pInstance;
}

Stats::Stats() : num_trans_sent(0),
				num_trans_recv(0),
				num_trans_lost(0),
				acum_time_trans(SC_ZERO_TIME),
				num_data_pck_created(0),
				num_data_pck_recv(0),
				num_ack_pck_recv(0),
				num_tries(0),
				acum_pck_delay(SC_ZERO_TIME) {
	// TODO Auto-generated constructor stub
	ch_stats.resize(NUM_CHANNELS);

}


void Stats::printStats(){

	cout << "-------------------- WSN SIMULATION STATISTICS --------------------" << endl
		<< "Simulation time: " << sc_time_stamp().to_seconds() << " sec" << endl
		<< "Execution Time: " << simulation_time << " sec" << endl
		<< "#Motes: " << NUM_MOTES << endl
		<< "#Channels: " << NUM_CHANNELS << endl
		<< "MAX_ITERATIONS: " << MAX_ITERATIONS << endl
		<< "MAX_BITRATE: " << MAX_BITRATE << " kbps" << endl
		<< "-------------------------- PHYSICAL LAYER --------------------------" << endl
		<< "Maximum IDLE Period: " << sc_time(MAX_IDLE_TIME, TIME_UNITS) << endl
		<< "#Packets sent: " << num_trans_sent << endl
		<< "#Packets received: " << num_trans_recv << endl
		<< "Received Rate: " << get_ratio(num_trans_recv, num_trans_sent) << "%" << endl
		<< "#Collided Transactions: " << num_trans_lost << endl
		<< "Collision Rate: " << get_ratio(num_trans_lost, num_trans_sent) << "%" << endl
		<< "Average packets sent per mote: " << ( (float)num_trans_sent/(float)NUM_MOTES ) << endl
		<< "Average collided packets per mote: " << ( (float)num_trans_lost/(float)NUM_MOTES ) << endl
		<< "Average transmission delay per packet: " << (acum_time_trans/(double)num_trans_sent) << endl
		<< endl;
	if(MAC_ENABLE){
	cout<< "----------------------------- MAC LAYER ---------------------------" << endl << endl
		<< "#Transactions created: " << num_data_pck_created << endl
		<< "#DATA Packets received: " << num_data_pck_recv << endl
		<< "#ACK_Packets received: " << num_ack_pck_recv << endl
		<< "ACK Ratio: " << get_ratio(num_ack_pck_recv, num_data_pck_created) << "%" << endl
		<< "Average tries per transaction: " << ( (float)num_tries/(float)num_data_pck_created ) << endl
		<< "Average delay per try: " << ( acum_pck_delay/(double)num_tries ) << endl
		<< "Backoff time (MAC): " << Mote_a::get_backoff_time() << endl
		<< endl;
	}
	for(unsigned int i = 0; i < ch_stats.size(); i++){
	cout<< "---------------------------<<< Channel " << i << " >>>-------------------------" << std::endl << std::endl
		<< "Channel BitRate: " << channel_bitrate_v[i] << " kbps" << endl
		<< "Transmission time (4 bytes per transaction): " << get_delay(i, 4) << endl
		<< "#Transactions: " << ch_stats[i].num_trans_sent << endl
		<< "#Collided Transactions: " << ch_stats[i].num_trans_lost << endl
		<< "Channel Collision Rate: " << get_ratio(ch_stats[i].num_trans_lost, ch_stats[i].num_trans_sent) << "%"
		<< endl;
	}

}

ofstream& Stats::printStats(ofstream& ret){

	ret << "-------------------- WSN SIMULATION STATISTICS --------------------" << endl
		<< "Simulation time: " << sc_time_stamp().to_seconds() << " sec" << endl
		<< "Execution Time: " << simulation_time << " sec" << endl
		<< "#Motes: " << NUM_MOTES << endl
		<< "#Channels: " << NUM_CHANNELS << endl
		<< "MAX_ITERATIONS: " << MAX_ITERATIONS << endl
		<< "MAX_BITRATE: " << MAX_BITRATE << " kbps" << endl
		<< "----------------------- PHYSICAL LAYER STATS -----------------------" << endl << endl
		<< "Maximum Idle Period: " << sc_time(MAX_IDLE_TIME, TIME_UNITS) << endl
		<< "#Transactions sent: " << num_trans_sent << endl
		<< "#Transactions received: " << num_trans_recv << endl
		<< "Received Rate: " << get_ratio(num_trans_recv, num_trans_sent) << "%" << endl
		<< "#Collided Transactions: " << num_trans_lost << endl
		<< "Collision Rate: " << get_ratio(num_trans_lost, num_trans_sent) << "%" << endl
		<< "Average of Transactions per mote: " << ( (float)num_trans_sent/(float)NUM_MOTES ) << endl
		<< "Average of Collisions per mote: " << ( (float)num_trans_lost/(float)NUM_MOTES ) << endl
		<< "Average time per transaction: " << (acum_time_trans/(double)num_trans_sent) << endl
		<< endl;
	if(MAC_ENABLE){
		ret << "------------------------- MAC LAYER STATS -----------------------" << endl << endl
			<< "#DATA Packets created: " << num_data_pck_created << endl
			<< "#DATA Packets received: " << num_data_pck_recv << endl
			<< "#ACK Packets received: " << num_ack_pck_recv << endl
			<< "ACK Ratio: " << get_ratio(num_ack_pck_recv, num_data_pck_created) << "%" << endl
			<< "Average tries per created packet: " << ( (float)num_tries/(float)num_data_pck_created ) << endl
			<< "Average delay per try: " << ( acum_pck_delay/(double)num_tries ) << endl
			<< "Backoff time (MAC): " << Mote_a::get_backoff_time() << endl
			<< endl;
	}

	for(unsigned int i = 0; i < ch_stats.size(); i++){
		 ret << "---------------------<<< Channel " << i << " >>>---------------------" << std::endl << std::endl
			 << "Channel BitRate: " << channel_bitrate_v[i] << " kbps" << endl
			 << "Transmission time (4 bytes per transaction): " << get_delay(i, 4) << endl
			 << "#Transactions: " << ch_stats[i].num_trans_sent << endl
			 << "#Collided Transactions: " << ch_stats[i].num_trans_lost << endl
			 << "Channel Collision Rate: " << get_ratio(ch_stats[i].num_trans_lost, ch_stats[i].num_trans_sent) << "%"
			 << endl;
	};
	return ret;
}

inline float Stats::get_ratio(int a, int b){
	float f = 0;
	if(b != 0)
		f = ((float)a / (float)b )* 100;
	return f;
}

void Stats::log_Stats(){
	Log* log_stats = Log::Instance_Stats();

	log_stats->SC_log() << printStats(log_stats->get_ofstream());

}


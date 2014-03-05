/*
 * Stats.h
 *
 *  Created on: Mar 31, 2014
 *      Author: anubiscsc
 */

#ifndef STATS_H_
#define STATS_H_

#include <iostream>
#include <vector>
#include <systemc.h>

class Stats {
public:
	struct channel_stats{
		int num_trans_sent; //total amount of transactions transmitted through the channel
		int num_trans_lost; //total amount of transactions collided in the channel
	};
	std::vector<channel_stats> ch_stats;
	//declaration of physical layer variables
	int num_trans_sent;
	int num_trans_recv;
	int num_trans_lost;
	sc_time acum_time_trans;

	//declaration of MAC layer variables
	int num_data_pck_created;
	int num_data_pck_recv;
	int num_ack_pck_recv;
	int num_tries;
	sc_time acum_pck_delay;
	sc_mutex mutex;

	static Stats* Instance();
	void printStats();
	ofstream& printStats(ofstream&);

	void log_Stats();
	float get_ratio(int, int);
	void set_nChannels(int);

protected:
	Stats();
	//Stats operator=(Stats const&);
	static Stats* m_pInstance;
};

#endif /* STATS_H_ */

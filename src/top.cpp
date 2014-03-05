//============================================================================
// Name        : Top.cpp
// Author      : Carlos Sánchez
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================


#include "top.h"
#include "mote_base.h"
#include "mote_a.h"
#include "mote_b.h"
#include "channel.h"
#include "log.h"
#include "stats.h"

#include <random>
#include <sys/time.h>
#include <ctime>

using namespace sc_core;
using namespace sc_dt;
using namespace std;

double simulation_time = 0;
std::vector<int> channel_bitrate_v;
sc_time_unit TIME_UNITS;
int MAX_BITRATE = 300; //in bits per second
double MAX_IDLE_TIME = 1500;
int NUM_CHANNELS = 0;
int NUM_MOTES = 0;
int BASE_MOTE_ID = -1;
int active_motes;
sc_event halt_event;

// Seed with a real random value, if available
std::random_device rd;
std::default_random_engine e0(rd());
std::mt19937_64 e1(rd());

//Packet delivery time = Transmission time + Propagation delay
//Transmission time depends on the packet size (in bytes) and the bitrate (in bit/s).
//We take into account just the transmission time because propagation delay is insignificant.
sc_time get_delay(const int& channelId, const int& packet_size){
	return sc_time( ( ((double)packet_size * 8) / (double)(channel_bitrate_v[channelId]*1000.0L) ) * 1000000.0L, SC_US);
}

Top::Top(sc_module_name name_) : sc_module(name_){
#if LOG_PHY_FLAG
	Log* log = Log::Instance_PHY();
#endif
	//Stats* stats = Stats::Instance();
	char txt[20];
	//std::uniform_int_distribution<int> distr(0,NUM_MOTES-1);
	channel1 = new Channel("channel");
#if LOG_PHY_FLAG
	log->SC_log() << "<<< Wireless Sensor Network (" << NUM_MOTES << " motes) >>>" << endl << endl;
	if(MAC_ENABLE){ log->SC_log() << "<<< MAC & PHYSICAL LAYER SIMULATION >>>" << endl << endl;}
	else{ log->SC_log() << "<<< PHYSICAL LAYER SIMULATION >>>" << endl << endl;}
#endif
	std::vector<Mote_base*> motes;
	if(MAC_ENABLE){
		std::vector<Mote_a*> motes;
		sprintf(txt, "mote_base");
		Mote_b* mote_b = new Mote_b(txt);
		mote_b->i_socket.bind(channel1->mpt_socket);
		channel1->mpi_socket.bind(mote_b->t_socket);
	}
	motes.resize(NUM_MOTES);
#if LOG_PHY_FLAG
	log->SC_log() << name() << ": Connecting motes with channel module" << endl;
	//BASE_MOTE_ID = distr(e0);
	log->SC_log() << name() << ": base_mote = mote_" << BASE_MOTE_ID << endl;
#endif
	for (int i = 0; i < NUM_MOTES; i++){
		sprintf(txt, "mote_%d", i);
		if(MAC_ENABLE) motes[i] = new Mote_a(txt, i);
		else motes[i] = new Mote_base(txt, i);
		motes.at(i)->i_socket.bind(channel1->mpt_socket);
		channel1->mpi_socket.bind(motes.at(i)->t_socket);
	}
	/* backoff time = maximum channel delay * 3 */
	Mote_a::SetMACParams(get_delay(0, 4)*BACKOFF_TIME_MULT, 5);
}


void init(){

//	sc_set_default_time_unit(1,SC_MS);
	sc_set_time_resolution( 1, TIME_UNITS);
// 	MAX_BITRATE *= 1000; /* Convert from kbps to bps */
	switch(TIME_UNITS){
//	case SC_PS:
//		MAX_IDLE_TIME *= 1000000000L;
//		break;
	case SC_NS:
		MAX_IDLE_TIME *= 1000000L;
		break;
	case SC_US:
		MAX_IDLE_TIME *= 1000L;
		break;
	case SC_SEC:
		MAX_IDLE_TIME /= 1000.0L;
		break;
	default:
		;
	}
	active_motes = NUM_MOTES;

}


int sc_main (int argc, char* argv[]) {

	// initialize global variables from command line parameters
	NUM_MOTES = (int)strtol(argv[1], 0, 0);
	NUM_CHANNELS = (int)strtol(argv[2], 0, 0);
	MAX_BITRATE = (int)strtol(argv[3], 0, 0);
	MAX_IDLE_TIME = (int)strtol(argv[4], 0, 0);
	TIME_UNITS = static_cast<sc_time_unit>(strtol(argv[5], 0, 0));

	init();

	Top top("top");

	Stats* stats = Stats::Instance();
	cout << "Start SystemC Simulation" << endl;

	struct timeval diff, startTV, endTV;
	gettimeofday(&startTV, NULL);

	/* Start Simulation */
	sc_start(); //Start Simulation

	gettimeofday(&endTV, NULL);
	timersub(&endTV, &startTV, &diff);
	/* Calculates the execution time of the whole simulation */
	simulation_time = ( (diff.tv_sec * 1000000L) + diff.tv_usec ) / 1000000.0L; /* time in ms */

#ifdef DEBUG
	printf( "Time in microseconds: %ld microseconds\n",
			((endTV.tv_sec - startTV.tv_sec)*1000000L + endTV.tv_usec) - startTV.tv_usec );
	cout << "Simulation time: " << simulation_time << " seconds" << endl;
#endif

	/* print Simulation Stattistics */
	stats->printStats(); //print the main simulation statistics on the default output
	stats->log_Stats(); //print the main simulation statistics on the STATS_log file

	return 0;
}

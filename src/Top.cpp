//============================================================================
// Name        : Top.cpp
// Author      : Carlos Sánchez
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include "Top.h"
#include "Log.h"
#include "channel.h"
#include "mote.h"

using namespace sc_core;
using namespace sc_dt;
using namespace std;

Top::Top(sc_module_name name_, int n_motes) : sc_module(name_){

	Log* log = Log::Instance();
	log->SC_log() << "<<< Wireless Sensor Network (" << n_motes << " motes) >>>" << endl << endl;

	mote* motes[n_motes];
	channel1 = new channel("channel1");
	char txt[20];

	log->SC_log() << "connecting motes with channel1..." << endl;
	for (int i = 0; i < n_motes; i++){
		sprintf(txt, "mote_%d", i);
		motes[i] = new mote(txt);

		motes[i]->i_socket.bind(channel1->mpt_socket);
		channel1->mpi_socket.bind(motes[i]->t_socket);
	}
}

int sc_main (int argc, char* argv[]) {

	int size = (int)strtol(argv[1], 0, 0);
	Top top("top", size);
	sc_start();

	return 0;
}

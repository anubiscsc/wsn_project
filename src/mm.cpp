/*
 * mm.cpp
 *
 *  Created on: Feb 25, 2014
 *      Author: anubiscsc
 */

#include "mm.h"
#include "log.h"

mm::mm() : free_list(0), empties(0) {
#if LOG_PHY_FLAG
	log = Log::Instance_PHY();
#else
	log = 0;
#endif
}

mm::gp_t*  mm::allocate()
{
	gp_t* ptr;

	if (free_list){
		ptr = free_list->trans;
		empties = free_list;
		free_list = free_list->next;
	}
	else
		ptr = new gp_t(this);

	return ptr;
}

void mm::free(gp_t* trans)
{

	if (!empties){
		empties = new access;
		empties->next = free_list;
		empties->prev = 0;
		if (free_list)
			free_list->prev = empties;
	}
	free_list = empties;
	free_list->trans = trans;
	empties = free_list->prev;

#if LOG_PHY_FLAG
	if(trans->get_address() != 0) //Only display the messages of SENDING_TRANS motes
		log->SC_log() << "Memory Manager: Transaction (Address=" << hex << trans->get_address()
						<< dec << ") Released from channel" << endl;
#endif
}


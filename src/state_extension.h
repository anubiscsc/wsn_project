/*
 * state_extension.h
 *
 *  Created on: Feb 27, 2014
 *      Author: anubiscsc
 */

#ifndef STATE_EXTENSION_H_
#define STATE_EXTENSION_H_

#include <tlm_core/tlm_2/tlm_generic_payload/tlm_gp.h>
#include "mote_base.h"

/*
 * In case state=STANDBY or state=LISTENING_CHANNEL the mote will wait a random simulation time for the next state setting.
 */
/* DEPRECATED */
class state_extension: public tlm::tlm_extension<state_extension> {
public:
	Mote_base::state_t state; //Current state of the source mote
	bool has_changed; //indicates whether the mote state has changed or not
	bool force_change; //indicates to the source mote that the transaction wasn't sent to any mote (no LISTENING_CHANNEL motes were found)
	sc_event sending_change_event; //event that locks and unlocks the main Mote loop when the source mote state is SENDING_TRANS
	int test;

	state_extension();
	tlm::tlm_extension_base* clone() const;
	void copy_from(tlm_extension_base const&);
};

#endif /* STATE_EXTENSION_H_ */

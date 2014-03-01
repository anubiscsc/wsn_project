/*
 * state_extension.h
 *
 *  Created on: Feb 27, 2014
 *      Author: anubiscsc
 */

#ifndef STATE_EXTENSION_H_
#define STATE_EXTENSION_H_

#include <tlm_core/tlm_2/tlm_generic_payload/tlm_gp.h>

class state_extension: public tlm::tlm_extension<state_extension> {
public:
	state_extension();
	bool state_change;
	mote::state_t state;
	virtual tlm::tlm_extension_base* clone() const;
	virtual void copy_from(tlm_extension_base const&);
};

#endif /* STATE_EXTENSION_H_ */

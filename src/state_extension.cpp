/*
 * state_extension.cpp
 *
 *  Created on: Feb 27, 2014
 *      Author: anubiscsc
 */

#include "state_extension.h"

using namespace tlm;

state_extension::state_extension(): state(Mote::STANDBY), state_change(false) { } //Constructor

tlm_extension_base* state_extension::clone() const {
	state_extension* t = new state_extension();
	//t->ID = this->ID;
	t->state = this->state;
	t->state_change = this->state_change;
	return t;
}

void state_extension::copy_from(tlm_extension_base const &ext){
	//ID = static_cast< state_extension const & >(ext).ID;
	state = static_cast< state_extension const & >(ext).state;
	state_change = static_cast< state_extension const & >(ext).state_change;
}

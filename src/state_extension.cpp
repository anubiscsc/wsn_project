/*
 * state_extension.cpp
 *
 *  Created on: Feb 27, 2014
 *      Author: anubiscsc
 */

#include "state_extension.h"

using namespace tlm;

state_extension::state_extension(): state(Mote_base::IDLE), has_changed(false), force_change(false), test(NUM_MOTES-1) { } //Constructor

tlm_extension_base* state_extension::clone() const {
	state_extension* t = new state_extension();

	t->state = this->state;
	t->has_changed = this->has_changed;
	t->force_change = this->force_change;
	t->test = this->test;

	return t;
}

void state_extension::copy_from(tlm_extension_base const &ext){
	//ID = static_cast< state_extension const & >(ext).ID;
	state = static_cast< state_extension const & >(ext).state;
	has_changed = static_cast< state_extension const & >(ext).has_changed;
	force_change = static_cast< state_extension const & >(ext).force_change;
	test = static_cast< state_extension const & >(ext).test;
}

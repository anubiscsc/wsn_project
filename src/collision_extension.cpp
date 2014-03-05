/*
 * collision_extension.cpp
 *
 *  Created on: Mar 3, 2014
 *      Author: anubiscsc
 */

#include "collision_extension.h"

using namespace tlm;

collision_extension::collision_extension(): is_Collision(false) { } //Constructor

tlm_extension_base* collision_extension::clone() const {
	collision_extension* t = new collision_extension();
	t->is_Collision = this->is_Collision;
	return t;
}

void collision_extension::copy_from(tlm_extension_base const &ext){
	is_Collision = static_cast< collision_extension const & >(ext).is_Collision;
}

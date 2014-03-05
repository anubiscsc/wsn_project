/*
 * collision_extension.h
 *
 *  Created on: Mar 3, 2014
 *      Author: anubiscsc
 */

#ifndef COLLISION_EXTENSION_H_
#define COLLISION_EXTENSION_H_

#include <tlm_core/tlm_2/tlm_generic_payload/tlm_gp.h>

class collision_extension: public tlm::tlm_extension<collision_extension> {
public:
	collision_extension();
	bool is_Collision;
	tlm::tlm_extension_base* clone() const;
	void copy_from(tlm_extension_base const&);
};


#endif /* COLLISION_EXTENSION_H_ */

/*
 * channel_extension.h
 *
 *  Created on: Mar 6, 2014
 *      Author: anubiscsc
 */

#ifndef CHANNEL_EXTENSION_H_
#define CHANNEL_EXTENSION_H_

#include <tlm_core/tlm_2/tlm_generic_payload/tlm_gp.h>

class channel_extension: public tlm::tlm_extension<channel_extension> {
public:
	channel_extension();
	int id_channel;
	tlm::tlm_extension_base* clone() const;
	void copy_from(tlm_extension_base const&);
};


#endif /* CHANNEL_EXTENSION_H_ */

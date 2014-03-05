/*
 * channel_extension.cpp
 *
 *  Created on: Mar 6, 2014
 *      Author: anubiscsc
 */

#include "channel_extension.h"

using namespace tlm;

channel_extension::channel_extension() : id_channel(0){
	// TODO Auto-generated constructor stub
}

tlm_extension_base* channel_extension::clone() const {
	channel_extension* t = new channel_extension();
	t->id_channel = this->id_channel;
	return t;
}

void channel_extension::copy_from(tlm_extension_base const &ext){
	id_channel = static_cast< channel_extension const & >(ext).id_channel;
}


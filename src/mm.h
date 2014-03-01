/*
 * mm.h
 *
 *  Created on: Feb 25, 2014
 *      Author: anubiscsc
 */

#ifndef MM_H_
#define MM_H_

//#include "mote.h"

#include "tlm_utils/simple_initiator_socket.h"

using namespace sc_core;
using namespace std;

class mm: public tlm::tlm_mm_interface
{
  typedef tlm::tlm_generic_payload gp_t;

public:
  mm();

  gp_t* allocate();
  void  free(gp_t*);

private:
  struct access
  {
    gp_t* trans;
    access* next;
    access* prev;
  };

  access* free_list;
  access* empties;

};

#endif /* MM_H_ */

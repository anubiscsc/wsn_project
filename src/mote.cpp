/*
 * mote.cpp
 *
 *  Created on: 03/02/2014
 *      Author: Anubiscsc
 */

#include "mote.h"
#include "Log.h"

using namespace sc_core;
using namespace sc_dt;
using namespace std;

mote::mote(sc_module_name name_) :
			sc_module(name_),
			i_socket("socket"),
			t_socket("_socket"),
			response_in_progress(false),
			request_in_progress(0),
			m_state(mote::STANDBY),
			m_peq(this, &mote::peq_cb) {

	i_socket.register_nb_transport_bw(this, &mote::nb_transport_bw);
	t_socket.register_nb_transport_fw(this, &mote::nb_transport_fw);
	//t_socket.register_transport_dbg(this, &mote::transport_dbg);

	log = Log::Instance();

	pos.x = rand()%100;
	pos.y = rand()%100;
	pos.z = rand()%100;

	SC_THREAD(thread_process);

}

void mote::thread_process(){

	tlm::tlm_generic_payload* trans;
	tlm::tlm_phase phase;
	sc_time delay;
	tlm::tlm_sync_enum status;
	state_extension *s_ext = new state_extension(); //Instanciate a new extension object

	//Initialize generic payload attributes
	trans->set_command(tlm::TLM_IGNORE_COMMAND);
	//...

	//generate a state behaviour changing

	switch (s_ext->state) {

	case (mote::STANDBY):  //if this->m_state=STANDBY

		if (s_ext->state_change){
			//status = nb_transport_fw(...);
		}


	case (mote::LISTENING_CHANNEL):  //if this->m_state=LISTENING_CHANNEL

		// ...

	case (mote::SENDING_TRANS):	 //if this->m_state=SENDING_TRANS

		// Generate a sequence of random transactions
		for (int i = 0; i < 2; i++)
		{
			tlm::tlm_command cmd = static_cast<tlm::tlm_command>(rand() % 2);
			if (cmd == tlm::TLM_WRITE_COMMAND) buffer[i % 10] = rand();

			//cout << name() << ": cmd=" << cmd << endl;
			// Grab a new transaction from the memory manager
			trans = m_mm.allocate();
			// Increment the transaction reference count
			trans->acquire();

			// Set all attributes except byte_enable_length and extensions (unused)
			trans->set_command( cmd );
			trans->set_address( rand() );
			trans->set_data_ptr( reinterpret_cast<unsigned char*>(&buffer[i % 10]) );
			trans->set_data_length( 4 );
			trans->set_streaming_width( 4 ); // = data_length to indicate no streaming
			trans->set_byte_enable_ptr( 0 ); // 0 indicates unused
			trans->set_dmi_allowed( false ); // Mandatory initial value
			trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value
			trans->set_extension(s_ext); //set a new extension to the generic payload

			// Initiator must honor BEGIN_REQ/END_REQ exclusion rule
			if (request_in_progress)
				wait(end_request_event);

			request_in_progress = trans;
			phase = tlm::BEGIN_REQ;
			delay = sc_time(rand()%50, SC_NS);

			log->SC_log() << "Initiator " << name() << " ----> Sending Transaction through channel1" << endl;
			m_state = mote::SENDING_TRANS; //sets the mote state to SENDING TRANSACTION
			status = i_socket->nb_transport_fw(*trans, phase, delay); // Non-blocking transport call on the forward path
			m_state = mote::STANDBY; //sets the mote state to STANDBY
			if (status == tlm::TLM_UPDATED){
				//insert a transaction into the Payload Event Queue for
				//sum of simulation time plus delay time
				m_peq.notify(*trans, phase, delay);
			}
			else if(status == tlm::TLM_COMPLETED){
				// The completion of the transaction necessarily ends the BEGIN_REQ phase
				request_in_progress = 0;
				// The target has terminated the transaction
				check_transaction( *trans );
			}
		}

	}
}

tlm::tlm_sync_enum mote::nb_transport_fw( tlm::tlm_generic_payload& trans,
                                              	  tlm::tlm_phase& phase, sc_time& delay ){

	//sc_dt::uint64    adr = trans.get_address();
    unsigned int     len = trans.get_data_length();
    unsigned char*   byt = trans.get_byte_enable_ptr();
    unsigned int     wid = trans.get_streaming_width();

    // Obliged to check the transaction attributes for unsupported features
    // and to generate the appropriate error response
    if (byt != 0)
      trans.set_response_status( tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE );
    else if (len > 4 || wid < len)
      trans.set_response_status( tlm::TLM_BURST_ERROR_RESPONSE );
    else
    	trans.set_response_status(tlm::TLM_OK_RESPONSE);

    // Now queue the transaction until the annotated time has elapsed
    m_peq.notify( trans, phase, delay);
    return tlm::TLM_COMPLETED;
}

// Called on receiving BEGIN_RESP or TLM_COMPLETED
void mote::check_transaction(tlm::tlm_generic_payload& trans){

	if(trans.is_response_error()){
		//char txt[100];
		//sprintf(txt, "Transaction returned with error, response status = %s",
		//              trans.get_response_string().c_str());
		//SC_REPORT_ERROR("TLM-2", txt);
		log->SC_log() << "TLM-2 Transaction returned with error, response status = "
				<< trans.get_response_string().c_str() << endl;
	}

	tlm::tlm_command cmd = trans.get_command();
	uint64    		 adr = trans.get_address();
	int*             ptr = reinterpret_cast<int*>( trans.get_data_ptr() );

#ifdef DEBUG
	log->SC_log() << "DEBUG: " << name() << ": Transaction COMPLETED" << endl;
	log->SC_log() << "DEBUG: " << name() << ": Checking Transaction | Address=" << hex << adr
				<< " | cmd=" << (cmd ? 'W' : 'R') << " | data=" << hex << *ptr << endl;
#endif


	// Allow the memory manager to free the transaction object
	trans.release();
}
/*
void mote::listen_channel(){

	while(true){
		wait(); //wait for clock positive edge

	}

}
*/

/*
void mote::init(int ln){
	acum = NULL;
	FILE * file;
	file = new FILE("data");
	length = ln;
	file->_read(file, buffer, 6);

}
*/

// Payload event queue callback to handle transactions from target
// Transaction could have arrived through return path or backward path

void mote::peq_cb(tlm::tlm_generic_payload& trans, const tlm::tlm_phase& phase){

	#ifdef DEBUG
		if (phase == tlm::END_REQ)
			log->SC_log() << "DEBUG: " <<  name() << " | address: " << hex << trans.get_address() << " | phase=END_REQ" << endl;
		else if (phase == tlm::BEGIN_RESP)
			log->SC_log() << "DEBUG: " <<  name() << " | address: " << hex << trans.get_address() << " | phase=BEGIN_RESP"<< endl;
		else if (phase == tlm::BEGIN_REQ)
			log->SC_log() << "DEBUG: " <<  name() << " | address: " << hex << trans.get_address() << " | phase=BEGIN_REQ" << endl;
	#endif

	tlm::tlm_phase fw_phase = tlm::BEGIN_REQ;
    //tlm::tlm_sync_enum status;
    sc_time delay;

    switch (phase) {
    	//CASES FOR TARGET PAYLOAD EVENT QUEUE
    	case tlm::BEGIN_REQ:

			log->SC_log() << "Target " << name() << " <---- Transaction object received. | cmd=" << (trans.get_command()? 'W' : 'R')
							<< " | Address=" << hex << trans.get_address() << " | data=" << hex << *reinterpret_cast<int*>( trans.get_data_ptr() ) << endl;

			break;

		case tlm::END_RESP:
		// On receiving END_RESP, the target can release the transaction
		// and allow other pending transactions to proceed

			if (!response_in_progress)
				SC_REPORT_FATAL("TLM-2", "Illegal transaction phase END_RESP received by target");

			//trans.release();

			// Target itself is now clear to issue the next BEGIN_RESP
			response_in_progress = false;

			break;

		//CASES FOR INITIATOR PAYLOAD EVENT QUEUE
		case tlm::BEGIN_RESP:
			if(&trans == request_in_progress){
				// The end of the BEGIN_REQ phase
				request_in_progress = 0;
				end_request_event.notify();
			}

			check_transaction( trans );
			// Send final phase transition to target
			fw_phase = tlm::END_RESP;
			delay = sc_time(rand()%50, SC_NS);
			i_socket->nb_transport_fw( trans, fw_phase, delay );
			// Ignore return value
			break;

		case tlm::END_REQ:
			// The end of the BEGIN_REQ phase
			request_in_progress = 0;
			end_request_event.notify();

			log->SC_log() << "TLM_OK_RESPONSE" << endl;
			trans.set_response_status( tlm::TLM_OK_RESPONSE );

			// Target must honor BEGIN_RESP/END_RESP exclusion rule
			// i.e. must not send BEGIN_RESP until receiving previous END_RESP or BEGIN_REQ
			if (!response_in_progress)
				send_response(trans);

			break;

    }

}

tlm::tlm_sync_enum mote::send_end_req(tlm::tlm_generic_payload& trans){

	tlm::tlm_sync_enum status;
    tlm::tlm_phase bw_phase;
    //tlm::tlm_phase int_phase = internal_ph;
    sc_time delay;

    // Queue the acceptance and the response with the appropriate latency
    bw_phase = tlm::END_REQ;
    delay = sc_time(rand()%50, SC_NS); // Accept delay
    status = t_socket->nb_transport_bw( trans, bw_phase, delay );
    if (status == tlm::TLM_COMPLETED)
    {
		// Transaction aborted by the initiator
		// (TLM_UPDATED cannot occur at this point in the base protocol, so need not be checked)
		trans.release();
		return status;
    }
/*
    // Queue internal event to mark beginning of response
    delay = delay + sc_time(rand()%50, SC_NS); // Latency
    m_peq.notify( trans, int_phase, delay );
    n_trans++;
*/
    return status;
}

void mote::send_response(tlm::tlm_generic_payload& trans){

	tlm::tlm_sync_enum status;
	tlm::tlm_phase bw_phase;
	sc_time delay;

	response_in_progress = true;
	bw_phase = tlm::BEGIN_RESP;
	delay = SC_ZERO_TIME;
	status = t_socket->nb_transport_bw( trans, bw_phase, delay );

	if (status == tlm::TLM_UPDATED)
	{
	  // The timing annotation must be honored
	  m_peq.notify( trans, bw_phase, delay);
	}
	else if (status == tlm::TLM_COMPLETED)
	{
	  // The initiator has terminated the transaction
	  trans.release();
	  //n_trans--;
	  response_in_progress = false;
	}
}

tlm::tlm_sync_enum mote::nb_transport_bw( tlm::tlm_generic_payload& trans,
                                              tlm::tlm_phase& phase, sc_time& delay )
  {
    // The timing annotation must be honored
    m_peq.notify( trans, phase, delay );
    return tlm::TLM_ACCEPTED;
  }

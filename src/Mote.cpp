/*
 * mote.cpp
 *
 *  Created on: 03/02/2014
 *      Author: Anubiscsc
 */

#include "Mote.h"
#include "Log.h"
#include "state_extension.h"
#include "collision_extension.h"

using namespace sc_core;
using namespace sc_dt;
using namespace std;


ostream& operator<<(ostream& out, const Mote::state_t& state){
	switch (state){
		case Mote::LISTENING_CHANNEL: return out << "LISTENING_CHANNEL";
		case Mote::STANDBY: return out << "STANDBY";
		case Mote::SENDING_TRANS: return out << "SENDING_TRANS";
	}
}

Mote::Mote(sc_module_name name_) :
			sc_module(name_),
			i_socket("socket"),
			t_socket("_socket"),
			response_in_progress(false),
			request_in_progress(0),
			//m_state(Mote::STANDBY),
			m_peq(this, &Mote::peq_cb) {

	i_socket.register_nb_transport_bw(this, &Mote::nb_transport_bw);
	t_socket.register_nb_transport_fw(this, &Mote::nb_transport_fw);
	//t_socket.register_transport_dbg(this, &mote::transport_dbg);

	log = Log::Instance();

	pos.x = rand()%100;
	pos.y = rand()%100;
	pos.z = rand()%100;

	SC_THREAD(thread_process);

}

void Mote::thread_process(){

	tlm::tlm_generic_payload* trans;
	tlm::tlm_phase phase;
	sc_time delay;
	tlm::tlm_sync_enum status;
	state_extension *s_ext = new state_extension(); //Instanciate a new extension object
	collision_extension* c_ext = new collision_extension(); //Instanciate a new extension object
	Mote::state_t last_state;

	phase = tlm::BEGIN_REQ;
	delay = SC_ZERO_TIME; //sets delay variable to zero
	last_state = Mote::STANDBY; //starts with state=STANDBY because state_extension class constructor do so.

	trans = m_mm.allocate(); // Grab a new transaction from the memory manager
	trans->acquire(); // Increment the transaction reference count

	trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value
	trans->set_extension(s_ext); //set a new extension to the generic payload with state_extension type
	status = i_socket->nb_transport_fw(*trans, phase, delay); //Sending an initial transaction to update the channel states snapshot
	if(status == tlm::TLM_COMPLETED){
		request_in_progress = 0; // The completion of the transaction necessarily ends the BEGIN_REQ phase
		check_transaction( *trans ); // The target has terminated the transaction
	}

	for (int i = 0; i < 20; i++){ //repeat the process i times
		wait(sc_time(rand()%1000, SC_US)); //each iteration, the mote waits a random time to create asynchron mote behaviour

		s_ext->state = static_cast<Mote::state_t>(rand() % 3); //generate a random state behaviour


		if(last_state != s_ext->state)
			s_ext->state_change = true;
		else
			s_ext->state_change = false;

		//m_state = Mote::SENDING_TRANS; //sets the mote state to SENDING TRANSACTION
		log->SC_log() << "Initiator " << name() << " State=" << s_ext->state << endl;
		log->SC_log() << "Initiator " << name() << " ----> Sending gp_t object through channel1" << endl;

		tlm::tlm_command cmd = static_cast<tlm::tlm_command>(rand() % 2);
		if (cmd == tlm::TLM_WRITE_COMMAND) buffer[i % 10] = rand();

		trans = m_mm.allocate(); // Grab a new transaction from the memory manager
		trans->acquire(); // Increment the transaction reference count

		if(s_ext->state == Mote::SENDING_TRANS){
			// Set all attributes except byte_enable_length and extensions (unused)
			trans->set_command( cmd );
			trans->set_address( rand() );
			trans->set_data_ptr( reinterpret_cast<unsigned char*>(&buffer[i % 10]) );
			trans->set_data_length( 4 );
			trans->set_streaming_width( 4 ); // = data_length to indicate no streaming
			trans->set_byte_enable_ptr( 0 ); // 0 indicates unused
			trans->set_dmi_allowed( false ); // Mandatory initial value
			trans->set_extension(c_ext); //set a new extension to the generic payload with collision_extension type
		}
		trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value
		trans->set_extension(s_ext); //set a new extension to the generic payload with state_extension type


		// Initiator must honor BEGIN_REQ/END_REQ exclusion rule
		if (request_in_progress)
			wait(end_request_event);

		request_in_progress = trans;

		status = i_socket->nb_transport_fw(*trans, phase, delay); // Non-blocking transport call on the forward path

		if (status == tlm::TLM_UPDATED){
			//insert a transaction into the Payload Event Queue for
			//sum of simulation time plus delay time
			m_peq.notify(*trans, phase, delay);
		}
		else if(status == tlm::TLM_COMPLETED){
			request_in_progress = 0; // The completion of the transaction necessarily ends the BEGIN_REQ phase
			check_transaction( *trans ); // The target has terminated the transaction
		}
		last_state = s_ext->state;
	}
}

tlm::tlm_sync_enum Mote::nb_transport_fw( tlm::tlm_generic_payload& trans,
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
void Mote::check_transaction(tlm::tlm_generic_payload& trans){

	if(trans.is_response_error()){
		//char txt[100];
		//sprintf(txt, "Transaction returned with error, response status = %s",
		//              trans.get_response_string().c_str());
		//SC_REPORT_ERROR("TLM-2", txt);
		log->SC_log() << "TLM-2 Transaction returned with error, response status: "
				<< trans.get_response_string().c_str() << endl;
	}

#ifdef DEBUG
	tlm::tlm_command cmd = trans.get_command();
	uint64    		 adr = trans.get_address();
	int*             ptr = reinterpret_cast<int*>( trans.get_data_ptr() );

	log->SC_log() << "DEBUG: " << name() << ": Transaction COMPLETED" << endl;
	log->SC_log() << "DEBUG: " << name() << ": Checking Transaction | Address=" << hex << adr
				<< " | cmd=" << (cmd ? 'W' : 'R') << " | data=" << hex << *ptr << endl;
#endif

	trans.release(); // Allow the memory manager to free the transaction object
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

void Mote::peq_cb(tlm::tlm_generic_payload& trans, const tlm::tlm_phase& phase){

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
    collision_extension* c_ext;

    trans.get_extension(c_ext);

    switch (phase) {
    	//CASES FOR TARGET PAYLOAD EVENT QUEUE
    	case tlm::BEGIN_REQ:
    		if (c_ext->is_Collision)
			log->SC_log() << "Target " << name() << " <---- Transaction object received with an in-flight collision. | cmd=" << (trans.get_command()? 'W' : 'R')
							<< " | Address=" << hex << trans.get_address() << " | data=" << hex << *reinterpret_cast<int*>( trans.get_data_ptr() ) << endl;
    		else
    			log->SC_log() << "Target " << name() << " <---- Transaction object received. | cmd=" << (trans.get_command()? 'W' : 'R')
    						<< " | Address=" << hex << trans.get_address() << " | data=" << *reinterpret_cast<int*>( trans.get_data_ptr() ) << endl;
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

tlm::tlm_sync_enum Mote::send_end_req(tlm::tlm_generic_payload& trans){

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

void Mote::send_response(tlm::tlm_generic_payload& trans){

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

tlm::tlm_sync_enum Mote::nb_transport_bw( tlm::tlm_generic_payload& trans,
                                              tlm::tlm_phase& phase, sc_time& delay )
  {
    // The timing annotation must be honored
    m_peq.notify( trans, phase, delay );
    return tlm::TLM_ACCEPTED;
  }


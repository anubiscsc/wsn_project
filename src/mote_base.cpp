/*
 * mote_base.cpp
 *
 *  Created on: 03/02/2014
 *      Author: Anubiscsc
 */

#include "mote_base.h"
#include "state_extension.h"
#include "collision_extension.h"
#include "channel_extension.h"
#include "log.h"
#include "stats.h"

using namespace sc_core;
using namespace sc_dt;
using namespace std;

ostream& operator<<(ostream& out, const Mote_base::state_t& state){
	switch (state){
		case Mote_base::LISTENING: return out << "LISTENING";
		case Mote_base::IDLE: return out << "IDLE";
		case Mote_base::SENDING: return out << "SENDING";
		default: return out << "UNKNOWN";
	}
}

Mote_base::Mote_base(sc_module_name _name, int id) :
			i_socket("initiator_socket"),
			t_socket("target_socket"),
			mote_id(id),
			listening_channel_id(-1), //set to a non-posible value
			response_in_progress(false),
			request_in_progress(0),
			current_transaction(0),
			m_peq(this, &Mote_base::peq_cb),
			send_ack_peq(this, &Mote_base::send_response),
			module_name(_name),
			prev_state(Mote_base::IDLE),
			buffer(0)
			//sending_uniform_dist(0, MAX_IDLE_TIME)
			{

	i_socket.register_nb_transport_bw(this, &Mote_base::nb_transport_bw);
	t_socket.register_nb_transport_fw(this, &Mote_base::nb_transport_fw);
#if LOG_PHY_FLAG
	log = Log::Instance_PHY();
#else
	log = 0;
#endif
#if LOG_MAC_FLAG
	log_mac = Log::Instance_MAC();
#else
	log_mac = 0;
#endif
	stats = Stats::Instance();

}

string& Mote_base::get_name(){ return module_name;}

void Mote_base::update_channel(Mote_base::state_t state){

	tlm::tlm_generic_payload* trans;
	tlm::tlm_phase phase;
	sc_time delay;
	tlm::tlm_sync_enum status;
	state_extension *s_ext = new state_extension(); //Instanciate a new extension object

	s_ext->state = state; //generate a random state behaviour
	s_ext->has_changed = true;

	trans = m_mm.allocate(); // Grab a new transaction from the memory manager
	trans->acquire(); // Increment the transaction reference count

	trans->set_address(0);
	trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value
	trans->set_auto_extension(s_ext); //set a new extension to the generic payload with state_extension type
	// send transaction to channel through initiator socket
	status = i_socket->nb_transport_fw(*trans, phase, delay); // Non-blocking transport call on the forward path
	if(status == tlm::TLM_COMPLETED){
		prev_state = s_ext->state;
	}
	check_transaction(*trans); // The target has terminated the transaction
	trans->release();
}

tlm::tlm_sync_enum Mote_base::nb_transport_fw( tlm::tlm_generic_payload& trans,
                                              	  tlm::tlm_phase& phase, sc_time& delay ){

	//sc_dt::uint64    adr = trans.get_address();
    unsigned int     len = trans.get_data_length();
    unsigned char*   byt = trans.get_byte_enable_ptr();
    unsigned int     wid = trans.get_streaming_width();

    // Obliged to check the transaction attributes for unsupported features
    // and to generate the appropriate error response
    if (byt != 0)
      trans.set_response_status( tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE );
    else if (len > 8 || wid < len){
    	cout << "data_length=" << len << " | streaming_width=" << wid << endl;
    	trans.set_response_status( tlm::TLM_BURST_ERROR_RESPONSE );
    }

    else
    	trans.set_response_status(tlm::TLM_OK_RESPONSE);

    // Now queue the transaction until the annotated time has elapsed
    m_peq.notify( trans, phase, delay);
    return tlm::TLM_ACCEPTED;
}

// Called on receiving BEGIN_RESP or TLM_COMPLETED
void Mote_base::check_transaction(tlm::tlm_generic_payload& trans){

	if(trans.is_response_error()){
		char txt[100];
		sprintf(txt, "Transaction returned with error, response status = %s",
		              trans.get_response_string().c_str());
		SC_REPORT_ERROR("TLM-2", txt);
#if LOG_PHY_FLAG
		log->SC_log() << "TLM-2 Transaction returned with error, response status: "
				<< trans.get_response_string().c_str() << endl;
#endif
	}
}


// Payload event queue callback to handle transactions from target
// Transaction could have arrived through return path or backward path
void Mote_base::peq_cb(tlm::tlm_generic_payload& trans, const tlm::tlm_phase& phase){

#ifdef DEBUG
	if (phase == tlm::END_REQ)
		log->SC_log() << "DEBUG: " <<  name() << " | address: " << hex << trans.get_address() << dec << " | phase=END_REQ" << endl;
	else if (phase == tlm::BEGIN_RESP)
		log->SC_log() << "DEBUG: " <<  name() << " | address: " << hex << trans.get_address() << dec << " | phase=BEGIN_RESP"<< endl;
	else if (phase == tlm::BEGIN_REQ)
		log->SC_log() << "DEBUG: " <<  name() << " | address: " << hex << trans.get_address() << dec << " | phase=BEGIN_REQ" << endl;
#endif

    sc_time delay;
    collision_extension* c_ext;
    state_extension* s_ext;
    channel_extension *ch_ext;// = new channel_extension();

    trans.get_extension(c_ext);
    trans.get_extension(s_ext);
    trans.get_extension(ch_ext);

    switch (phase) {
    	//CASES FOR TARGET PAYLOAD EVENT QUEUE
    	case tlm::BEGIN_REQ:
				if (c_ext->is_Collision == true){
#if LOG_PHY_FLAG
					bool print = true;
					if(MAC_ENABLE && get_name() != "base_mote"){
							print = false;
					}

					if(print){
						log->SC_log() << "Target " << get_name() << " <---- Transaction object received with an in-flight collision. | cmd=" << (trans.get_command()? 'W' : 'R')
									  << " | Address=" << hex << trans.get_address() << " | data=" << hex << *reinterpret_cast<int*>( trans.get_data_ptr() ) << dec << endl;
					}
#else
					;
#endif
				}
				else{
					buffer = reinterpret_cast<int*>( trans.get_data_ptr() );
#if LOG_PHY_FLAG
					log->SC_log() << "Target " << get_name() << " <---- Transaction object received from channel " << ch_ext->id_channel << " | cmd=" << (trans.get_command()? 'W' : 'R')
												<< " | Address=" << hex << trans.get_address() << " | data=" << *buffer << dec << endl;
#endif
					if(MAC_ENABLE && mote_id == BASE_MOTE_ID){
						listening_channel_id = ch_ext->id_channel;
#if LOG_MAC_FLAG
						log_mac->SC_log() << "Target " << get_name() << " <---- Packet received | Address=" << hex << trans.get_address() << " | data=" << *buffer << dec << endl;
#endif
						stats->num_trans_recv += 1;
						stats->num_data_pck_recv++;
						/* The base_mote instance variable "current_transaction" gets the last transaction received */
						current_transaction = &trans;
						trans_recv_event.notify();
					}
				}
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

			if(current_transaction && ch_ext->id_channel == listening_channel_id) { //check whether the mote is trying to commit a transaction or not, and if the incomming transaction is being sent through the correct channel
				if(c_ext->is_Collision){
#if LOG_PHY_FLAG
					log->SC_log() << "Target " << get_name() << " <---- Transaction object received from channel " << ch_ext->id_channel << " with an in-flight collision | PHASE=BEGIN_RESP | cmd=" << (trans.get_command()? 'W' : 'R')
								  << " | Address=" << hex << trans.get_address() << " | data=" << *reinterpret_cast<int*>( trans.get_data_ptr() ) << dec << endl;
#else
					;
#endif
				}
				else{
#if LOG_PHY_FLAG
					log->SC_log() << "Target " << get_name() << " <---- Transaction object received from channel " << ch_ext->id_channel << " | PHASE=BEGIN_RESP | cmd=" << (trans.get_command()? 'W' : 'R')
								  << " | Address=" << hex << trans.get_address() << " | data=" << string(trans.get_data_ptr(), trans.get_data_ptr() + trans.get_data_length() ) << dec << endl;
#endif

					if(trans.get_address() == current_transaction->get_address()){ //if the ack transaction received is the right one, then
						stats->num_trans_recv += 1;
						stats->num_ack_pck_recv++;
#if LOG_MAC_FLAG
						log_mac->SC_log() << "Initiator " << get_name() << " <---- ACK Packet received | Phase=BEGIN_RESP" << endl;
#endif
						ack_recv_event.notify();// warns that the ack packet has been received ...by the source mote

					}
				}
			}

			/* decrement counter in order to release the transaction */
			trans.release(); //decrements the trans.m_ref_count private variable
			break;

		case tlm::END_REQ:
			// The end of the BEGIN_REQ phase
			request_in_progress = 0;
			end_request_event.notify();
#if LOG_PHY_FLAG
			log->SC_log() << "TLM_OK_RESPONSE" << endl;
#endif
			trans.set_response_status( tlm::TLM_OK_RESPONSE );

			// Target must honor BEGIN_RESP/END_RESP exclusion rule
			// i.e. must not send BEGIN_RESP until receiving previous END_RESP or BEGIN_REQ
			if (!response_in_progress)
				//send_response(trans);

			break;

    }

}

tlm::tlm_sync_enum Mote_base::send_end_req(tlm::tlm_generic_payload& trans){

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
    }
/*
    // Queue internal event to mark beginning of response
    delay = delay + sc_time(rand()%50, SC_NS); // Latency
    m_peq.notify( trans, int_phase, delay );
    n_trans++;
*/
    return status;
}

void Mote_base::send_response(tlm::tlm_generic_payload& trans, const tlm::tlm_phase& phase){

	tlm::tlm_sync_enum status;
	tlm::tlm_phase bw_phase;
	sc_time delay;
	state_extension *s_ext = new state_extension(); //Instanciate a new extension object
	channel_extension *ch_ext = new channel_extension();
	unsigned char* data = new unsigned char[3]{'A', 'C', 'K'};

	request_in_progress = 0; //indicates that the request phase has finished
	response_in_progress = true; //indicates that response phase begins
	bw_phase = phase;
	delay = SC_ZERO_TIME;
	ch_ext->id_channel = listening_channel_id;
	s_ext->state = Mote_base::SENDING; //generate a random state behaviour

	if(prev_state != s_ext->state)
		s_ext->has_changed = true;
	else
		s_ext->has_changed = false;

	if(trans.get_ref_count() == 0) trans.acquire(); // Increment the transaction reference count
//	tlm::tlm_command cmd = static_cast<tlm::tlm_command>(rand() % 2);
	/* Set all attributes except byte_enable_length and extensions (unused) */
//	trans->set_command( cmd );
//  trans->set_address( rand() );
	trans.set_data_ptr(data); //sets "ACK" as packet data
	trans.set_data_length( 3 );
	trans.set_streaming_width( 3 ); // = data_length to indicate no streaming
//	trans->set_byte_enable_ptr( 0 ); // 0 indicates unused
//	trans->set_dmi_allowed( false ); // Mandatory initial value
	trans.set_auto_extension(ch_ext); //adds a channel_extension object to the extensions array
	trans.set_auto_extension(s_ext); //set a new extension to the generic payload with state_extension type
	trans.set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value
#if LOG_MAC_FLAG
	log_mac->SC_log() << "Target " << get_name() << " ----> Sending back ACK Packet | Phase=BEGIN_RESP" << endl;
#endif
	status = t_socket->nb_transport_bw( trans, bw_phase, delay );

	if (status == tlm::TLM_UPDATED)
	{
	  // The timing annotation must be honored
	  m_peq.notify( trans, bw_phase, delay);
	}
	else if (status == tlm::TLM_COMPLETED)
	{
		// The initiator has terminated the transaction
		check_transaction(trans); // The target has terminated the transaction
		trans.release();

		response_in_progress = false;
	}
}

tlm::tlm_sync_enum Mote_base::nb_transport_bw( tlm::tlm_generic_payload& trans,
                                              tlm::tlm_phase& phase, sc_time& delay )
{
    // Insert in payload event queue this transaction object with corresponding delay
    m_peq.notify( trans, phase, delay );
    return tlm::TLM_COMPLETED;
}

//sends a new transaction (tlm_generic_payload) to the channel
void Mote_base::send(int channelId, int* data, int data_size)
{
	tlm::tlm_generic_payload* trans;
	tlm::tlm_phase phase;
	sc_time delay;
	tlm::tlm_sync_enum status;
	state_extension *s_ext = new state_extension(); //Instanciate a new extension object
	channel_extension *ch_ext = new channel_extension();

	phase = tlm::BEGIN_REQ;
	delay = SC_ZERO_TIME; //sets delay variable to zero

	ch_ext->id_channel = channelId;
	s_ext->state = Mote_base::SENDING; //generate a random state behaviour

	if(prev_state != s_ext->state)
		s_ext->has_changed = true;
	else
		s_ext->has_changed = false;

	if(current_transaction == 0){

		trans = m_mm.allocate(); // Grab a new transaction from the memory manager
		tlm::tlm_command cmd = static_cast<tlm::tlm_command>(rand() % 2);
		// Set all attributes except byte_enable_length and extensions (unused)
		trans->set_command( cmd );
		trans->set_address( rand() );
		trans->set_data_ptr( reinterpret_cast<unsigned char*>(data) );
		trans->set_data_length( data_size );
		trans->set_streaming_width( data_size ); // = data_length to indicate no streaming
		trans->set_byte_enable_ptr( TLM_BYTE_DISABLED ); // 0 indicates unused
		trans->set_dmi_allowed( false ); // Mandatory initial value
		trans->set_auto_extension(ch_ext); //adds a channel_extension object to the extensions array
		trans->set_auto_extension(s_ext); //set a new extension to the generic payload with state_extension type
		trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value

		current_transaction = trans;
	}
	else{
		trans = current_transaction;
	}
	if(trans->get_ref_count() < 1)
		trans->acquire(); // Increment the transaction reference count

	// send transaction to channel through initiator socket
	status = i_socket->nb_transport_fw(*trans, phase, delay); // Non-blocking transport call on the forward path

	if (status == tlm::TLM_UPDATED){
		//insert a transaction into the Payload Event Queue for sum of simulation time plus delay time
		m_peq.notify(*trans, phase, delay);
	}
	else if(status == tlm::TLM_COMPLETED){
		prev_state = s_ext->state;
		if(!s_ext->force_change)
			wait(s_ext->sending_change_event); //waits until the last mote which received the transaction warns through the extension array

		request_in_progress = 0; // The completion of the transaction necessarily ends the BEGIN_REQ phase
		current_transaction = 0;
		check_transaction(*trans); // The target has terminated the transaction
		trans->release();

	}
}

//receive data from channel "channel_id" and stores it in a buffer
void Mote_base::recv(int channelId, int& buffer_received, int buffer_size){

	do{
#if LOG_PHY_FLAG
		log->SC_log() << "Target " << get_name() << ": listening through channel " << listening_channel_id << " | buffer_size=" << buffer_size << endl;
#endif
		/* wait until the mote gets (at "peq_cb" function) the required non-collided data through channel id,
		 * or gets the halt signal for finishing its execution (active_motes=1) */
		wait(trans_recv_event | halt_event);
		buffer_size = buffer_size - 4;
	}while(buffer_size != 0);

	buffer = 0;
	if(MAC_ENABLE){
		/* Set an ACK Packet delay according to the bitrate of channel */
		sc_time ack_delay(get_delay(listening_channel_id, 4).to_double(), TIME_UNITS);
		send_ack_peq.notify(*current_transaction, tlm::BEGIN_RESP, ack_delay); //insert in the payload event queue the current transaction to be handled after the ack delay.
	}
}


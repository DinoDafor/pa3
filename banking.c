#include "banking.h"
#include "utils.h"
#include "general_structures.h"

void transfer(void * parent_data, local_id src, local_id dst, balance_t amount) {

    proc_inf_t* processInformation = parent_data;

    Message* message = create_message(get_payload_from_transfer(src, dst, amount),TRANSFER);

    send(processInformation, src, message);

    do {
        if (receive(processInformation, dst, message) == 0) {
            set_lamport_time(message->s_header.s_local_time);
            increment_lamport_time();
        }
    } while (message->s_header.s_type != ACK);
}

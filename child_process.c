#include <stdlib.h>
#include <string.h>
#include "child_process.h"
#include "utils.h"


void work_stage_child_process(proc_inf_t* processInformation) {

    close_unused_pipes(processInformation);

    char* payload = print_output(log_started_fmt, processInformation, NULL, STARTED);
    Message* message = create_message(payload, STARTED);
    send_multicast((void*) processInformation, message);
    free(payload);
    free(message);

    while (processInformation->environment->numOfProcess - 1 !=
           get_live_process_count(processInformation->numbOfLiveP, processInformation->environment->numOfProcess)) {
        receive_from_children(processInformation);
    }

    payload = print_output(log_received_all_started_fmt, processInformation, NULL, ANY_TYPE);
    free(payload);

    while (processInformation->signalStopGet != 1) {
        handle_messages(processInformation);
    }

    payload = print_output(log_done_fmt, processInformation, NULL, DONE);
    message = create_message(payload, DONE);
    send_multicast(processInformation, message);
    free(payload);
    free(message);

    while (get_live_process_count(processInformation->numbOfLiveP, processInformation->environment->numOfProcess) != 1) {
        receive_from_children(processInformation);
    }

    payload = print_output(log_received_all_done_fmt, processInformation, NULL, ANY_TYPE);

    free(payload);

    message = create_message(NULL, BALANCE_HISTORY);
    memcpy(message->s_payload, processInformation->historyOfBalance, sizeof(BalanceHistory));
    send(processInformation, 0, message);

    exit(0);
}

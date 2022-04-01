#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "utils.h"
#include "common.h"

void set_default_state_live_processes(const int local_id, int* pProcesses, int process_count) {

    pProcesses[0] = 1;

    for (int process_id = 1; process_id < process_count; process_id ++) {
        if (local_id != process_id) pProcesses[process_id] = 0;
        else pProcesses[process_id] = 1;
    }

}


evn_t* init_environment(int process_num) {

    evn_t* env = malloc(sizeof(evn_t));

    env->numOfProcess = process_num;

    pippes_t* table = calloc(process_num * process_num, sizeof(pippes_t));
    pippes_t** row = malloc(process_num * sizeof(pippes_t));

    for (int row_index = 0 ; row_index < process_num; row_index ++) {
        row[row_index] = table + process_num * row_index;
        for (int column_index = 0; column_index < process_num; column_index ++) {
            if (row_index != column_index) {
                pipe(row[row_index][column_index].fileDesc);
                fcntl(row[row_index][column_index].fileDesc[0], F_SETFL, O_NONBLOCK);
                fcntl(row[row_index][column_index].fileDesc[1], F_SETFL, O_NONBLOCK);
            }
        }
    }

    env->pPipes = row;

    env->logFD = open(events_log, O_APPEND | O_CREAT | O_RDWR);

    return env;
}


proc_inf_t* create_process(evn_t* environment, int local_id, int balance) {
    proc_inf_t* processInformation = malloc(sizeof(proc_inf_t));

    BalanceState  balanceState = {.s_balance = balance, .s_time = get_lamport_time(), .s_balance_pending_in = 0};

    processInformation->environment = environment;
    processInformation->localId = local_id;
    processInformation->signalStopGet = 0;
    processInformation->historyOfBalance = malloc(sizeof(BalanceHistory));
    processInformation->historyOfBalance->s_history_len = 1;
    processInformation->historyOfBalance->s_id = local_id;
    processInformation->historyOfBalance->s_history[0] = balanceState;
    processInformation->historyAll = NULL;
    processInformation->messageGet = 0;


    int* neighbor = calloc(environment->numOfProcess, sizeof(int));
    set_default_state_live_processes(processInformation->localId, neighbor, processInformation->environment->numOfProcess);
    processInformation->numbOfLiveP = neighbor;

    return processInformation;
}

void close_unused_pipes(proc_inf_t* processInformation) {

    evn_t* environment = processInformation->environment;

    for (int row = 0; row < environment->numOfProcess; row ++) {
        for (int column = 0; column < environment->numOfProcess; column ++) {
            if (row != column && row != processInformation->localId && column != processInformation->localId) {
                close(environment->pPipes[row][column].fileDesc[1]);
                close(environment->pPipes[row][column].fileDesc[0]);
            } else if (row == processInformation->localId) {
                close(environment->pPipes[row][column].fileDesc[0]);
            } else if (column == processInformation->localId) {
                close(environment->pPipes[row][column].fileDesc[1]);
            }
        }
    }

}

MessageHeader create_header(const char* payload, MessageType type) {
    increment_lamport_time();
    if (type == TRANSFER) {
        MessageHeader header = {
                .s_type = TRANSFER,
                .s_payload_len = sizeof(TransferOrder),
                .s_magic = MESSAGE_MAGIC,
                .s_local_time = get_lamport_time()
        };
        return header;
    } else if (type == BALANCE_HISTORY) {
        MessageHeader  header = {
                .s_type = BALANCE_HISTORY,
                .s_payload_len = sizeof(BalanceHistory),
                .s_magic = MESSAGE_MAGIC,
                .s_local_time = get_lamport_time()
        };
        return header;
    } else {
        MessageHeader header = {
                .s_local_time = get_lamport_time(),
                .s_payload_len = payload != NULL ? strlen(payload) : 0,
                .s_magic = MESSAGE_MAGIC,
                .s_type = type
        };

        return header;
    }
}

Message* create_message(const char* payload, MessageType type) {

    MessageHeader  header = create_header(payload, type);

    Message* message = (Message*) malloc(MAX_MESSAGE_LEN);
    message->s_header = header;

    if (payload != NULL && BALANCE_HISTORY != type) {
        strncpy(message->s_payload, payload, strlen(payload));
    }

    return message;
}

char* print_output(const char* message_format, const proc_inf_t* processInformation, const TransferOrder* transferOrder, MessageType type) {
    char* payload = malloc(strlen(message_format));
    switch (type) {
        case STARTED:
            sprintf(payload, message_format, get_lamport_time(), processInformation->localId, getpid(), getppid(), processInformation->historyOfBalance->s_history[processInformation->historyOfBalance->s_history_len - 1].s_balance);
            break;
        case DONE:
            sprintf(payload, message_format, get_lamport_time(), processInformation->localId, processInformation->historyOfBalance->s_history[processInformation->historyOfBalance->s_history_len - 1].s_balance);
            break;
        case TRANSFER:
            if (transferOrder->s_src == processInformation->localId)
                sprintf(payload, message_format, get_lamport_time(), processInformation->localId, transferOrder->s_amount, transferOrder->s_dst);
            else
                sprintf(payload, message_format, get_lamport_time(), processInformation->localId, transferOrder->s_amount, transferOrder->s_src);
            break;
        default:
            sprintf(payload, message_format, get_lamport_time(), processInformation->localId);
    }

    write(processInformation->environment->logFD, payload, strlen(payload));
    printf("%s", payload);
    return payload;

}

int get_live_process_count(int* processes, int process_count) {

    int working_processes = 0;

    for (int i = 1; i < process_count; i ++) {
        if (processes[i] == 1) working_processes ++;
    }

    return working_processes;
}

void update_balance_history(const TransferOrder* transferOrder, BalanceHistory* balanceHistory, int increase_mode, int message_time) {

    int system_time = get_lamport_time();

    while (balanceHistory->s_history_len < system_time) {
        BalanceState balanceState = {
                .s_balance_pending_in = increase_mode == 1 && balanceHistory->s_history_len >= message_time - 1 ? transferOrder->s_amount : 0,
                .s_time = balanceHistory->s_history_len,
                .s_balance = balanceHistory->s_history[balanceHistory->s_history_len - 1].s_balance
        };
        balanceHistory->s_history[balanceHistory->s_history_len] = balanceState;
        balanceHistory->s_history_len ++;
    }

    BalanceState balanceState = {
            .s_balance_pending_in = 0,
            .s_time = get_lamport_time(),
            .s_balance = increase_mode == 1 ?
                    balanceHistory->s_history[balanceHistory->s_history_len - 1].s_balance + transferOrder->s_amount:
                    balanceHistory->s_history[balanceHistory->s_history_len - 1].s_balance - transferOrder->s_amount
    };

    balanceHistory->s_history[balanceHistory->s_history_len] = balanceState;
    balanceHistory->s_history_len ++;

}


void get_transfer_from_parent(const Message* message, proc_inf_t* processInformation) {

    TransferOrder* transferOrder = (TransferOrder*) message->s_payload;

    update_balance_history(transferOrder, processInformation->historyOfBalance, 0, message->s_header.s_local_time);

    print_output(log_transfer_out_fmt, processInformation, transferOrder, TRANSFER);

    Message* msg = create_message(get_payload_from_transfer(processInformation->localId, transferOrder->s_dst, transferOrder->s_amount), TRANSFER);

    send(processInformation, transferOrder->s_dst, msg);

}


void get_transfer_from_child(const Message* message, proc_inf_t* processInformation) {

    TransferOrder* transferOrder = (TransferOrder*) message->s_payload;

    update_balance_history(transferOrder, processInformation->historyOfBalance, 1, message->s_header.s_local_time);

    print_output(log_transfer_in_fmt, processInformation, transferOrder, TRANSFER);

    Message* msg = create_message(NULL, ACK);

    send(processInformation, 0, msg);
}


void handle_messages(proc_inf_t* processInformation) {

    Message  message;
    int i = 0;
    while (i < processInformation->environment->numOfProcess) {
        if (i != processInformation->localId) {
            if (receive(processInformation, i, &message) == 0) {
                switch (message.s_header.s_type) {
                    case STOP:
                        set_lamport_time(message.s_header.s_local_time);
                        increment_lamport_time();
                        processInformation->signalStopGet = 1;
                        break;
                    case TRANSFER:
                        set_lamport_time(message.s_header.s_local_time);
                        increment_lamport_time();
                        if (i == 0) {
                            get_transfer_from_parent(&message, processInformation);
                        } else {
                            get_transfer_from_child(&message, processInformation);
                        }
                        break;
                }
            }
        }
        i ++;
    }
}


char* get_payload_from_transfer(const int src, const int dst, const int balance) {

    TransferOrder* transferOrder = malloc(sizeof(TransferOrder));
    transferOrder->s_dst = dst;
    transferOrder->s_src = src;
    transferOrder->s_amount = balance;

    return (char*) transferOrder;
}

void update_all_history(const int from_local_id, const BalanceHistory* balanceHistory, AllHistory* allHistory) {
    int max_length = balanceHistory->s_history_len > allHistory->s_history_len ? balanceHistory->s_history_len : allHistory->s_history_len;
    allHistory->s_history_len = max_length;
    allHistory->s_history[from_local_id - 1] = *balanceHistory;


    for (int local_id = 0; local_id < from_local_id; local_id ++) {
        BalanceHistory old_balance_history = allHistory->s_history[local_id];


        while (old_balance_history.s_history_len < max_length) {
            BalanceState balanceState = {
                    .s_balance_pending_in = 0,
                    .s_time = old_balance_history.s_history_len,
                    .s_balance = old_balance_history.s_history[old_balance_history.s_history_len - 1].s_balance
            };
            old_balance_history.s_history[old_balance_history.s_history_len] = balanceState;
            old_balance_history.s_history_len ++;
        }
        allHistory->s_history[local_id] = old_balance_history;
    }
}

void receive_from_children(proc_inf_t* processInformation) {

    Message message;

    for (int local_id = 1; local_id < processInformation->environment->numOfProcess; local_id ++) {
        if (receive(processInformation, local_id, &message) == 0) {
            if (message.s_header.s_type == STARTED) {
                processInformation->numbOfLiveP[local_id] = 1;
                set_lamport_time(message.s_header.s_local_time);
                increment_lamport_time();
            } else if (message.s_header.s_type == DONE) {
                processInformation->numbOfLiveP[local_id] = 0;
                set_lamport_time(message.s_header.s_local_time);
                increment_lamport_time();
            } else if (message.s_header.s_type == BALANCE_HISTORY) {
                update_all_history(local_id, (BalanceHistory*) message.s_payload, processInformation->historyAll);
                processInformation->messageGet ++;
                set_lamport_time(message.s_header.s_local_time);
                increment_lamport_time();
            }
        }
    }
}

void send_children(proc_inf_t* processInformation, Message* message) {
    for (int local_id = 1; local_id < processInformation->environment->numOfProcess; local_id ++) {
        send(processInformation, local_id, message);
    }
}

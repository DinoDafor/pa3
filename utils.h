#ifndef PA1_UTILS_H
#define PA1_UTILS_H

#include "general_structures.h"
#include "malloc.h"
#include "pa2345.h"
#include "banking.h"
#include "ipc.h"

#define ANY_TYPE -1

void increment_lamport_time();
void set_lamport_time(timestamp_t timestamp);
evn_t* init_environment(int process_num);
proc_inf_t* create_process(evn_t* environment, int local_id, int balance);
void close_unused_pipes(proc_inf_t* processInformation);
Message* create_message(const char* payload, MessageType type);
void set_default_state_live_processes(const int local_id, int* pProcesses, int process_count);
char* print_output(const char* message_format, const proc_inf_t* processInformation, const TransferOrder* transferOrder, MessageType type);
int get_live_process_count(int* processes, int process_count);
void receive_from_children(proc_inf_t* processInformation);
char* get_payload_from_transfer(const int src, const int dst, const int balance);
void handle_messages(proc_inf_t* processInformation);
void send_children(proc_inf_t* processInformation, Message* message);
#endif //PA1_UTILS_H

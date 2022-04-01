#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include "general_structures.h"
#include "utils.h"
#include "stdlib.h"
#include "child_process.h"

#define SKIP_FIRST_ARGUMENTS 2

static timestamp_t current_time = 0;

timestamp_t get_lamport_time() {
    return current_time;
}

void increment_lamport_time() {
    current_time ++;
}

void set_lamport_time(timestamp_t timestamp) {
    current_time = timestamp > current_time ? timestamp : current_time;
}


void work_stage_main_process(proc_inf_t* processInformation) {

    close_unused_pipes(processInformation);

    processInformation->historyAll = malloc(sizeof(AllHistory));
    processInformation->historyAll->s_history_len = 0;

    bank_robbery(processInformation, processInformation->environment->numOfProcess - 1);

    Message* message = create_message(NULL, STOP);

    send_children(processInformation, message);

    while (waitpid(-1, NULL, 0 ) != -1);

    while (processInformation->messageGet != processInformation->environment->numOfProcess - 1) {
        receive_from_children(processInformation);
    }

    processInformation->historyAll->s_history_len = processInformation->environment->numOfProcess - 1;
    print_history(processInformation->historyAll);

}

int main(int argc, char* argv[]) {
    int process_count;
    sscanf(argv[2], "%d", &process_count);

    evn_t* environment = init_environment(process_count + 1);

    __pid_t process;
    for (int process_id = 1; process_id < environment->numOfProcess; process_id ++) {
        if ((process = fork()) == 0) {
            proc_inf_t* processInformation = create_process(environment, process_id, atoi(argv[process_id + SKIP_FIRST_ARGUMENTS]));
            work_stage_child_process(processInformation);
        }
    }

    proc_inf_t* processInformation = create_process(environment, 0, 0);

    work_stage_main_process(processInformation);

    close(processInformation->environment->logFD);


    return 0;
}

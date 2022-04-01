#ifndef PA1_GENERAL_STRUCTURES_H
#define PA1_GENERAL_STRUCTURES_H

#include "banking.h"

typedef enum {true, false} boolean;

struct pipes {
    int fileDesc[2];
} typedef pippes_t;

struct env {
    pippes_t** pPipes;
    int logFD;
    int numOfProcess;
} typedef evn_t;


struct proc_inf {
    evn_t* environment;
    int localId;
    int* numbOfLiveP;
    int signalStopGet;
    BalanceHistory* historyOfBalance;
    AllHistory* historyAll;
    int messageGet;
} typedef proc_inf_t;


#endif //PA1_GENERAL_STRUCTURES_H

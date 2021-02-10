#pragma once

#include "transaction.h"
#include "pocketdb/pocketdb.h"

class PocketTransaction : public CTransactionRef {
public:

    // social part of transaction
    PocketModel* PocketData;

    PocketTransaction();
    PocketTransaction(CTransaction tx);
    PocketTransaction(CMutableTransaction mtx);
    ~PocketTransaction();
};
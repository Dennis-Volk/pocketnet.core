#include "pockettransaction.h"

PocketTransaction::PocketTransaction() { }
PocketTransaction::PocketTransaction(CTransaction tx) : CTransactionRef(MakeTransactionRef(std::move(tx)))
{ }

PocketTransaction::PocketTransaction(CMutableTransaction mtx) : CTransactionRef(MakeTransactionRef(std::move(mtx)))
{ }

PocketTransaction::~PocketTransaction()
{ }


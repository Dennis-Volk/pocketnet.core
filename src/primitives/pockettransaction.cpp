#include "pockettransaction.h"

PocketTransaction::PocketTransaction() { }
PocketTransaction::PocketTransaction(CTransaction tx) : CTransactionRef(MakeTransactionRef(std::move(tx)))
{ }

PocketTransaction::PocketTransaction(CMutableTransaction mtx) : CTransactionRef(MakeTransactionRef(std::move(mtx)))
{ }

PocketTransaction::~PocketTransaction()
{ }

bool PocketTransaction::Deserialize(const std::string& data)
{
    // TODO (brangr): @@@ parse json to pocket model

//    UniValue _txs_src(UniValue::VOBJ);
//    _txs_src.read(pocket_data);
//
//    rtx.pTable = _txs_src["t"].get_str();
//    rtx.pTransaction = g_pocketdb->DB()->NewItem(rtx.pTable);
//    rtx.pTransaction.FromJSON(DecodeBase64(_txs_src["d"].get_str()));
//
//    if (rtx.pTable == "Mempool") {
//        rtx.pTable = rtx.pTransaction["table"].As<string>();
//        std::string _data = rtx.pTransaction["data"].As<string>();
//        rtx.pTransaction = g_pocketdb->DB()->NewItem(rtx.pTable);
//        rtx.pTransaction.FromJSON(DecodeBase64(_data));
//    }

    return false;
}

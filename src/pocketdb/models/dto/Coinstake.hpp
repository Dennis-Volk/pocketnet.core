// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_COINSTAKE_HPP
#define POCKETTX_COINSTAKE_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class Coinstake : public Transaction
    {
    public:

        Coinstake(string& hash, int64_t time, shared_ptr<string> opReturn) : Transaction(hash, time, opReturn)
        {
            SetType(PocketTxType::TX_COINSTAKE);
        }

        void Deserialize(const UniValue& src) override {}

    protected:

        void DeserializePayload(const UniValue& src) override {}
        void BuildHash() override {}

    };

} // namespace PocketTx

#endif // POCKETTX_COINSTAKE_HPP

// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_POCKETTAGSRPC_H
#define SRC_POCKETTAGSRPC_H

#include "rpc/server.h"
#include "rpc/rawtransaction.h"
#include "pocketdb/helpers/TransactionHelper.h"

namespace PocketWeb::PocketWebRpc
{
    //using namespace PocketHelpers;

    UniValue GetTags(const JSONRPCRequest& request);
}

#endif //SRC_POCKETTAGSRPC_H
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketScoresRpc.h"

namespace PocketWeb::PocketWebRpc
{
    UniValue GetAddressScores(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "getaddressscores\n"
                "\nGet scores from address.\n");

        std::string address;
        if (!request.params.empty() && request.params[0].isStr())
        {
            CTxDestination dest = DecodeDestination(request.params[0].get_str());

            if (!IsValidDestination(dest))
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid address: ") + request.params[0].get_str());

            address = request.params[0].get_str();
        }
        else
        {
            throw JSONRPCError(RPC_INVALID_PARAMS, "There is no address.");
        }

        vector<string> postHashes;
        if (request.params.size() > 1)
        {
            if (request.params[1].isArray())
            {
                UniValue txid = request.params[1].get_array();
                for (unsigned int idx = 0; idx < txid.size(); idx++)
                {
                    postHashes.push_back(txid[idx].get_str());
                }
            }
            else if (request.params[1].isStr())
            {
                postHashes.push_back(request.params[1].get_str());
            }
            else
            {
                throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid txs format");
            }
        }

        return request.DbConnection()->WebRepoInst->GetAddressScores(postHashes, address);
    }

    UniValue GetPostScores(const JSONRPCRequest& request)
    {
        //TODO mb add count param
        if (request.fHelp)
            throw std::runtime_error(
                "getpostscores\n"
                "\nGet scores from address.\n");

        vector<string> postHashes;
        if (!request.params.empty())
        {
            if (request.params[0].isArray())
            {
                UniValue txid = request.params[0].get_array();
                for (unsigned int idx = 0; idx < txid.size(); idx++)
                    postHashes.push_back(txid[idx].get_str());
            }
            else if (request.params[0].isStr())
                postHashes.push_back(request.params[0].get_str());
            else
                throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid txs format");
        }
        else
            throw JSONRPCError(RPC_INVALID_PARAMS, "There are no txid");


        std::string address;
        if (request.params.size() > 1 && request.params[1].isStr())
        {
            CTxDestination dest = DecodeDestination(request.params[1].get_str());
            if (!IsValidDestination(dest))
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid address: ") + request.params[1].get_str());

            address = request.params[1].get_str();
        }

        return request.DbConnection()->WebRepoInst->GetPostScores(postHashes, address);
    }

    UniValue GetPageScores(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "getpagescores\n"
                "\nGet scores for post(s) from address.\n");

        vector<string> uselessTxIds;
        if (!request.params.empty())
        {
            if (request.params[0].isArray())
            {
                UniValue txid = request.params[0].get_array();
                for (unsigned int idx = 0; idx < txid.size(); idx++)
                    uselessTxIds.push_back(txid[idx].get_str());
            }
            else if (request.params[0].isStr())
                uselessTxIds.push_back(request.params[0].get_str());
            else
                throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid txs format");
        }
        else
            throw JSONRPCError(RPC_INVALID_PARAMS, "There are no txid");

        std::string address;
        if (request.params.size() > 1 && request.params[1].isStr())
        {
            CTxDestination dest = DecodeDestination(request.params[1].get_str());
            if (!IsValidDestination(dest))
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid address: ") + request.params[1].get_str());

            address = request.params[1].get_str();
        }
        else
            throw JSONRPCError(RPC_INVALID_PARAMS, "There is no address.");

        vector<string> commentHashes;
        if (request.params.size() > 2)
        {
            if (request.params[2].isArray())
            {
                UniValue cmntid = request.params[2].get_array();
                for (unsigned int id = 0; id < cmntid.size(); id++)
                    commentHashes.push_back(cmntid[id].get_str());
            }
            else if (request.params[2].isStr())
                commentHashes.push_back(request.params[2].get_str());
            else
                throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid cmntids format");
        }

        return request.DbConnection()->WebRepoInst->GetPageScores(commentHashes, address);
    }
}
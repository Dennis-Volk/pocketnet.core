// Copyright (c) 2021 PocketNet developers
// PocketDB general wrapper
//-----------------------------------------------------
#ifndef POCKETREPOSITORY_H
#define POCKETREPOSITORY_H
//-----------------------------------------------------
#include <string>
#include <sqlite3.h>
#include <uint256.h>
#include "util.h"
#include "primitives/block.h"
#include "pocketdb/pocketmodels.h"
#include "index/addrindex.h"

using namespace std;
//-----------------------------------------------------
// Models


//-----------------------------------------------------
class PocketRepository {
private:
    sqlite3 *db;

    bool exec(const string &sql);

    Mutex cs;
    map<uint256, PocketModel*> pocketDataCache;

public:
    explicit PocketRepository();

    ~PocketRepository();

    bool Init(const std::string& table = "ALL");

    bool Add(PocketModel* itm); // ?
    bool Add(PocketBlock& pocketBlock);

    // Transactions data cache
    bool GetCachedTransaction(const CTransaction& tx, PocketModel* itm);
    bool GetCachedTransactions(const CBlock& block, vector<PocketModel*>& itms);
    bool AddCachedTransaction(PocketModel* itm);
    bool AddCachedTransactions(vector<PocketModel*>& itms);
    bool RemoveCachedTransaction(const CTransaction& tx);
    bool RemoveCachedTransactions(const CBlock& block);
};

extern std::unique_ptr<PocketRepository> g_pocket_repository;
//-----------------------------------------------------
#endif // POCKETREPOSITORY_H
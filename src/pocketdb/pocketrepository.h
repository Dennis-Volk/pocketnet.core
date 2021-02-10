// Copyright (c) 2021 PocketNet developers
// PocketDB general wrapper
//-----------------------------------------------------
#ifndef SQLITE_H
#define SQLITE_H
//-----------------------------------------------------
#include <string>
#include <sqlite3.h>
#include <uint256.h>
#include "util.h"
#include "pocketdb/pocketmodels.h"

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

    bool Add(PocketModel* itm);

    PocketModel* GetCachedTransaction(uint256 hash);
    void AddCachedTransaction(PocketModel* itm);
    void AddCachedTransactions(vector<PocketModel*> itms);
    void RemoveCachedTransaction(uint256 hash);
};

extern std::unique_ptr<PocketRepository> g_pocket_repository;
//-----------------------------------------------------
#endif // SQLITE_H
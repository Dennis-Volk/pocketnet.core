// Copyright (c) 2021 PocketNet developers
// PocketDB general wrapper
//-----------------------------------------------------
#ifndef SQLITE_H
#define SQLITE_H
//-----------------------------------------------------
#include <string>
#include <sqlite3.h>
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



public:
    explicit PocketRepository();

    ~PocketRepository();

    bool Init(const std::string& table = "ALL");

    bool Add(PocketModel* itm);
};

extern std::unique_ptr<PocketRepository> g_pocket_repository;
//-----------------------------------------------------
#endif // SQLITE_H
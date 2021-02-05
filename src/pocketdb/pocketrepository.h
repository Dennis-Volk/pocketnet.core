// Copyright (c) 2021 PocketNet developers
// PocketDB general wrapper
//-----------------------------------------------------
#ifndef SQLITE_H
#define SQLITE_H
//-----------------------------------------------------
#include <string>
#include <regex>
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

    static string sql(const string &sql) {
        return regex_replace(sql, regex("'"), "''");
    }

public:
    explicit PocketRepository();

    ~PocketRepository();

    bool Init(const std::string& table = "ALL");

    bool Add(const Utxo &itm);

    bool Add(const User &itm);

    bool Add(const Post &itm);

    bool Add(const PostScore &itm);

    bool Add(const Subscribe &itm);

    bool Add(const Blocking &itm);

    bool Add(const Complain &itm);

    bool Add(const Comment &itm);

    bool Add(const CommentScore &itm);

    // Runtime benchmark
    bool Add(const Checkpoint &itm);
};

extern std::unique_ptr<PocketRepository> g_pocket_repository;
//-----------------------------------------------------
#endif // SQLITE_H
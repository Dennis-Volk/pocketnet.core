// Copyright (c) 2021 PocketNet developers
// PocketDB general wrapper
//-----------------------------------------------------
#ifndef SQLITE_H
#define SQLITE_H
//-----------------------------------------------------
#include <string>
#include <sqlite3.h>

using namespace std;
//-----------------------------------------------------
// Models

struct UserView {
    string address;
    int id;
    string txid;
    int block;
    int time;
    string name;
    int birthday;
    int gender;
    int regdate;
    string avatar;
    string about;
    string lang;
    string url;
    string pubkey;
    string donations;
    string referrer;
    int reputation;
};

struct User {
    string address;
    int id;
    string txid;
    int block;
    int time;
    string name;
    int birthday;
    int gender;
    int regdate;
    string avatar;
    string about;
    string lang;
    string url;
    string pubkey;
    string donations;
    string referrer;
};

struct Utxo {
    string txid;
    int txout;
    int64_t time;
    int block;
    string address;
    int64_t amount;
    int spent_block;
};

//-----------------------------------------------------
class SqliteRepository {
private:
    sqlite3* db;
    bool _exec(string sql);

public:
    SqliteRepository(sqlite3* db);
    ~SqliteRepository();

    bool Add(Utxo utxo);
};
//-----------------------------------------------------
#endif // SQLITE_H
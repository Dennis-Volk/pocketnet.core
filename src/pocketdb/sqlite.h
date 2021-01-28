// Copyright (c) 2021 PocketNet developers
// PocketDB general wrapper
//-----------------------------------------------------
#ifndef SQLITE_H
#define SQLITE_H
//-----------------------------------------------------
#include <string>
#include <regex>
#include <sqlite3.h>

using namespace std;
//-----------------------------------------------------
// Models

struct UserView {
    string Address;
    int Id;
    string Txid;
    int Block;
    int Time;
    string Name;
    int Birthday;
    int Gender;
    int RegDate;
    string Avatar;
    string About;
    string Lang;
    string Url;
    string Pubkey;
    string Donations;
    string Referrer;
    int Reputation;
};

struct User {
    string Address;
    int Id;
    string Txid;
    int Block;
    int Time;
    string Name;
    int Birthday;
    int Gender;
    int RegDate;
    string Avatar;
    string About;
    string Lang;
    string Url;
    string Pubkey;
    string Donations;
    string Referrer;
};

struct Utxo {
    string Txid;
    int Txout;
    int64_t Time;
    int Block;
    string Address;
    int64_t Amount;
    int SpentBlock;
};

struct Mempool {
    string Txid;
    string TxidSource;
    string Table;
    string Data;
};

struct Service {
    int Version;
};

struct UserRatings {
    int Block;
    string Address;
    int Reputation;
};

struct Posts {
    string Txid;
    string TxidEdit;
    string TxidRepost;
    int Block;
    int Time;
    string Address;
    int Type;
    string Lang;
    string Caption;
    string Message;
    string Tags;
    string Url;
    string Images;
    string Settings;
    int ScoreSum;
    int ScoreCnt;
    int Reputation;
};

struct PostsHistory {
    string Txid;
    string TxidEdit;
    string TxidRepost;
    int Block;
    int Time;
    string Address;
    int Type;
    string Lang;
    string Caption;
    string Message;
    string Tags;
    string Url;
    string Images;
    string Settings;
};

struct PostRatings {
    int Block;
    string PostTxid;
    int ScoreSum;
    int ScoreCnt;
    int Reputation;
};

struct Scores {
    string Txid;
    int Block;
    int Time;
    string PostTxid;
    string Address;
    int Value;
};

struct SubscribesView {
    string Txid;
    int Block;
    int Time;
    string Address;
    string AddressTo;
    int Private;
};

struct Subscribes {
    string Txid;
    int Block;
    int Time;
    string Address;
    string AddressTo;
    int Private;
    int Ubsubscribe;
};

struct BlockingView {
    string Txid;
    int Block;
    int Time;
    string Address;
    string AddressTo;
    int AddressReputation;
};

struct Blocking {
    string Txid;
    int Block;
    int Time;
    string Address;
    string AddressTo;
    int Unblocking;
};

struct Complains {
    string Txid;
    int Block;
    int Time;
    string PostTxid;
    string Address;
    int Reason;
};

struct Addresses {
    string Txid;
    int Block;
    string Address;
    int Time;
};

struct Comment {
    string Txid;
    string OldTxid;
    int Last;
    string PostTxid;
    string Address;
    int Time;
    int Block;
    string Msg;
    string ParentId;
    string AnswerId;
    int ScoreUp;
    int ScoreDown;
    int Reputation;
};

struct CommentRatings {
    int Block;
    string CommentId;
    int ScoreUpl;
    int ScoreDown;
    int Reputation;
};

struct CommentScores {
    string Txid;
    int Block;
    int Time;
    string CommentId;
    string Address;
    int Value;
};


//-----------------------------------------------------
class SqliteRepository {
private:
    sqlite3* db;
    bool exec(string sql);
    string sql(string sql) {
        return regex_replace(sql, regex("'"), "''");
    }

public:
    SqliteRepository(sqlite3* db);
    ~SqliteRepository();

    bool Add(Utxo utxo);
};
//-----------------------------------------------------
#endif // SQLITE_H
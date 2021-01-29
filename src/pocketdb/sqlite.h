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

struct User {
    string Address;
    int Id;
    string Txid;
    int Block;
    int64_t Time;
    string Name;
    int Birthday;
    int Gender;
    int64_t RegDate;
    string Avatar;
    string About;
    string Lang;
    string Url;
    string Pubkey;
    string Donations;
    string Referrer;
    // int Reputation;
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

struct Post {
    string Txid;
    string TxidEdit;
    string TxidRepost;
    int Block;
    int64_t Time;
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

struct PostHistory {
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

struct PostRating {
    int Block;
    string PostTxid;
    int ScoreSum;
    int ScoreCnt;
    int Reputation;
};

struct PostScore {
    string Txid;
    int Block;
    int Time;
    string PostTxid;
    string Address;
    int Value;
};

struct SubscribeView {
    string Txid;
    int Block;
    int Time;
    string Address;
    string AddressTo;
    int Private;
};

struct Subscribe {
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

struct Complain {
    string Txid;
    int Block;
    int Time;
    string PostTxid;
    string Address;
    int Reason;
};

struct Address {
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

struct CommentRating {
    int Block;
    string CommentId;
    int ScoreUpl;
    int ScoreDown;
    int Reputation;
};

struct CommentScore {
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

    bool Add(Utxo itm);
    bool Add(User itm);
    bool Add(Post itm);
    bool Add(PostScore itm);
    bool Add(Subscribe itm);
    bool Add(Blocking itm);
    bool Add(Complain itm);
    bool Add(Comment itm);
    bool Add(CommentScore itm);
};
//-----------------------------------------------------
#endif // SQLITE_H
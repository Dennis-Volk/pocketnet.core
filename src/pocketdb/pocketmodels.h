//
// Created by Andrey Brunenko on 04.02.2021.
//

#ifndef SRC_POCKETMODELS_H
#define SRC_POCKETMODELS_H

#include <string>
#include <regex>

using namespace std;

static string sql(const string &sql) {
    return regex_replace(sql, regex("'"), "''");
}

enum MODELTYPE {
    USER,
    POST,
    POSTSCORE,
    COMMENT,
    COMMENTSCORE,
    SUBSCRIBE,
    COMPLAIN,
    BLOCKING,
};

struct PocketModel
{
    virtual MODELTYPE ModelType() = 0;
};


struct User : virtual PocketModel {
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

    MODELTYPE ModelType() override {
        return MODELTYPE::USER;
    }
};

struct Post : virtual PocketModel {
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

    MODELTYPE ModelType() override {
        return MODELTYPE::POST;
    }

//    string SqlInsert() override {
//        return tfm::format(
//            "insert into Posts ("
//            " Txid,"
//            " TxidEdit,"
//            " TxidRepost,"
//            " Block,"
//            " Time,"
//            " Address,"
//            " Type,"
//            " Lang,"
//            " Caption,"
//            " Message,"
//            " Tags,"
//            " Url,"
//            " Settings"
//            ") values ('%s',%d,'%s',%d,%ld,'%s',%d,%d,%ld,'%s','%s','%s','%s','%s','%s','%s');",
//            sql(Txid), sql(TxidEdit), sql(TxidRepost), Block, Time, sql(Address), Type, sql(Lang),
//            sql(Caption), sql(Message), sql(Tags), sql(Url), sql(Settings)
//        );
//    };
};

struct PostScore : virtual PocketModel {
    string Txid;
    int Block;
    int Time;
    string PostTxid;
    string Address;
    int Value;

    MODELTYPE ModelType() override {
        return MODELTYPE::POSTSCORE;
    }
};

struct Subscribe : virtual PocketModel {
    string Txid;
    int Block;
    int Time;
    string Address;
    string AddressTo;
    int Private;
    int Unsubscribe;

    MODELTYPE ModelType() override {
        return MODELTYPE::SUBSCRIBE;
    }
};

struct Blocking : virtual PocketModel {
    string Txid;
    int Block;
    int Time;
    string Address;
    string AddressTo;
    int Unblocking;
    int AddressReputation;

    MODELTYPE ModelType() override {
        return MODELTYPE::BLOCKING;
    }
};

struct Complain : virtual PocketModel {
    string Txid;
    int Block;
    int Time;
    string PostTxid;
    string Address;
    int Reason;

    MODELTYPE ModelType() override {
        return MODELTYPE::COMPLAIN;
    }
};

struct Comment : virtual PocketModel {
    string Txid;
    string TxidEdit;
    string PostTxid;
    string Address;
    int Time;
    int Block;
    string Msg;
    string ParentTxid;
    string AnswerTxid;
    int ScoreUp;
    int ScoreDown;
    int Reputation;

    MODELTYPE ModelType() override {
        return MODELTYPE::COMMENT;
    }
};

struct CommentScore : virtual PocketModel {
    string Txid;
    int Block;
    int Time;
    string CommentTxid;
    string Address;
    int Value;

    MODELTYPE ModelType() override {
        return MODELTYPE::COMMENTSCORE;
    }
};

// -----------------------------------
// Non transaction models

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
    string Model;
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

struct PostRating {
    int Block;
    string PostTxid;
    int ScoreSum;
    int ScoreCnt;
    int Reputation;
};

struct Address {
    string Txid;
    int Block;
    string Address;
    int Time;
};

struct CommentRating {
    int Block;
    string CommentId;
    int ScoreUpl;
    int ScoreDown;
    int Reputation;
};

// Benchmark model
struct Checkpoint {
    long long int Begin;
    long long int End;
    string Checkpoint;
    string Payload;

//    string SqlInsert() override {
//        return tfm::format(
//            " insert into Benchmark ("
//            "  Begin,"
//            "  End,"
//            "  Checkpoint,"
//            "  Payload"
//            " ) values (%lld,%lld,'%s','%s');",
//            Begin, End, sql(Checkpoint), sql(Payload)
//        );
//    }
};



#endif //SRC_POCKETMODELS_H

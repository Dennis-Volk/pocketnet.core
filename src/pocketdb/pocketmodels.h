//
// Created by Andrey Brunenko on 04.02.2021.
//

#ifndef SRC_POCKETMODELS_H
#define SRC_POCKETMODELS_H

#include <string>

using namespace std;

struct PocketModel
{
};


struct User : PocketModel {
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

struct Utxo : PocketModel {
    string Txid;
    int Txout;
    int64_t Time;
    int Block;
    string Address;
    int64_t Amount;
    int SpentBlock;
};

struct Mempool : PocketModel {
    string Txid;
    string TxidSource;
    string Model;
    string Data;
};

struct Service : PocketModel {
    int Version;
};

struct UserRatings : PocketModel {
    int Block;
    string Address;
    int Reputation;
};

struct Post : PocketModel {
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

struct PostHistory : PocketModel {
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

struct PostRating : PocketModel {
    int Block;
    string PostTxid;
    int ScoreSum;
    int ScoreCnt;
    int Reputation;
};

struct PostScore : PocketModel {
    string Txid;
    int Block;
    int Time;
    string PostTxid;
    string Address;
    int Value;
};

struct Subscribe : PocketModel {
    string Txid;
    int Block;
    int Time;
    string Address;
    string AddressTo;
    int Private;
    int Ubsubscribe;
};

struct Blocking : PocketModel {
    string Txid;
    int Block;
    int Time;
    string Address;
    string AddressTo;
    int Unblocking;
    int AddressReputation;
};

struct Complain : PocketModel {
    string Txid;
    int Block;
    int Time;
    string PostTxid;
    string Address;
    int Reason;
};

struct Address : PocketModel {
    string Txid;
    int Block;
    string Address;
    int Time;
};

struct Comment : PocketModel {
    string Txid;
    string TxidEdit;
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

struct CommentRating : PocketModel {
    int Block;
    string CommentId;
    int ScoreUpl;
    int ScoreDown;
    int Reputation;
};

struct CommentScore : PocketModel {
    string Txid;
    int Block;
    int Time;
    string CommentId;
    string Address;
    int Value;
};

// Benchmark model
struct Checkpoint : PocketModel {
    long long int Begin;
    long long int End;
    string Checkpoint;
    string Payload;
};



#endif //SRC_POCKETMODELS_H

// Copyright (c) 2021 PocketNet developers
// PocketDB general wrapper
//-----------------------------------------------------
#include "pocketrepository.h"
#include <tinyformat.h>

//-----------------------------------------------------

std::unique_ptr<PocketRepository> g_pocket_repository;

PocketRepository::PocketRepository() {
    if (sqlite3_open((GetDataDir() / "pocketdb" / "main.sqlite3").string().c_str(), &db)) {
        LogPrintf("Cannot open Sqlite DB (%s) - %s\n", (GetDataDir() / "pocketdb" / "general.sqlite3").string(),
                  sqlite3_errmsg(db));
        sqlite3_close(db);
    }
}

PocketRepository::~PocketRepository() {
    sqlite3_close(db);
}

//-----------------------------------------------------
bool PocketRepository::exec(const string &sql) {
    char *zErrMsg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &zErrMsg)) {
        // todo log
        sqlite3_free(zErrMsg);
        return false;
    }

    return true;
}
//-----------------------------------------------------

bool PocketRepository::Init(const string &table) {
    // Service
    if (table == "Service" || table == "ALL") {
        exec(" create table if not exists Service ("
             "  Service int primary key not null"
             " );"
        );
    }

    // CommentScores
    if (table == "Benchmark" || table == "ALL") {
        exec(" create table if not exists Benchmark ("
             "  Begin int not null,"
             "  End int not null,"
             "  Checkpoint text not null,"
             "  Payload text not null"
             " );"
             " truncate table Benchmark;"
        );
    }

    // Pocket Mempool
    if (table == "Mempool" || table == "ALL") {
        exec(" create table if not exists Mempool ("
             "  Txid text primary key not null,"
             "  TxidSource text,"
             "  Table text not null,"
             "  Data blob not null"
             " );"
        );
    }

    // Users
    if (table == "Users" || table == "ALL") {
        exec(" create table if not exists Users ("
             "  Address text primary key not null,"
             "  Id int,"
             "  Txid text not null,"
             "  Block int not null,"
             "  Time int not null,"
             "  Name text not null,"
             "  Birthday int,"
             "  Gender int,"
             "  RegDate int not null,"
             "  Avatar text,"
             "  About text,"
             "  Lang text,"
             "  Url text,"
             "  Pubkey text,"
             "  Donations text,"
             "  Referrer text"
             " );"
        );
    }

    // UserRatings
    if (table == "UserRatings" || table == "ALL") {
        exec(" create table if not exists UserRatings ("
             "  block int not null,"
             "  address text not null,"
             "  reputation int not null,"
             " primary key (address, block)"
             " );"
        );
    }

    // Posts
    if (table == "Posts" || table == "ALL") {
        exec(" create table if not exists Posts ("
             "  Txid text primary key not null,"
             "  TxidEdit text,"
             "  TxidRepost text,"
             "  Block int not null,"
             "  Time int not null,"
             "  Address text not null,"
             "  Type int,"
             "  Lang text,"
             "  Caption text,"
             "  Message text,"
             "  Tags text,"
             "  Url text,"
             "  Images text,"
             "  Settings text,"
             "  ScoreSum int,"
             "  ScoreCnt int,"
             "  Reputation int"
             " );"
        );
    }

    // Posts Hitstory
    if (table == "PostsHistory" || table == "ALL") {
        exec("create table if not exists PostsHistory ("
             "txid text not null,"
             "txidEdit text,"
             "txidRepost text,"
             "block int not null,"
             "time int not null,"
             "address text not null,"
             "type int,"
             "lang text,"
             "caption text,"
             "message text,"
             "tags text,"
             "url text,"
             "images text,"
             "settings text,"
             "primary key (txid, block)"
             ");"
        );
    }

    // PostRatings
    if (table == "PostRatings" || table == "ALL") {
        exec("create table if not exists PostRatings ("
             "block int not null,"
             "posttxid text not null,"
             "scoreSum text not null,"
             "scoreCnt int not null,"
             "reputation int not null,"
             "primary key (posttxid, block)"
             ");"
        );
    }

    // Scores
    if (table == "Scores" || table == "ALL") {
        exec("create table if not exists Scores ("
             "txid text primary key not null,"
             "block int not null,"
             "time int not null,"
             "posttxid text not null,"
             "address text not null,"
             "value int not null"
             ");"
        );
    }

    // Subscribes
    if (table == "SubscribesView" || table == "ALL") {
        exec("create table if not exists SubscribesView ("
             "txid text not null,"
             "block int not null,"
             "time int not null,"
             "address text not null,"
             "address_to text not null,"
             "private int not null,"
             "primary key (address, address_to)"
             ");"
        );
    }

    // RI SubscribesHistory
    if (table == "Subscribes" || table == "ALL") {
        exec("create table if not exists Subscribes ("
             "txid text primary key not null,"
             "block int not null,"
             "time int not null,"
             "address text not null,"
             "address_to text not null,"
             "private int not null,"
             "unsubscribe int not null"
             ");"
        );
    }

    // Blocking
    if (table == "BlockingView" || table == "ALL") {
        exec("create table if not exists BlockingView ("
             "txid text primary key not null,"
             "block int not null,"
             "time int not null,"
             "address text not null,"
             "address_to text not null,"
             "address_reputation int not null"
             ");"
        );
    }

    // BlockingHistory
    if (table == "Blocking" || table == "ALL") {
        exec("create table if not exists Blocking ("
             "txid text primary key not null,"
             "block int not null,"
             "time int not null,"
             "address text not null,"
             "address_to text not null,"
             "unblocking int not null"
             ");"
        );
    }

    // Complains
    if (table == "Complains" || table == "ALL") {
        exec("create table if not exists Complains ("
             "txid text primary key not null,"
             "block int not null,"
             "time int not null,"
             "posttxid text not null,"
             "address text not null,"
             "reason int not null"
             ");"
        );
    }

    // UTXO
    if (table == "Utxo" || table == "ALL") {
        exec("create table if not exists Utxo ("
             "txid text not null,"
             "txout int not null,"
             "time int not null,"
             "block int not null,"
             "address text not null,"
             "amount int not null,"
             "spent_block int,"
             "primary key (txid, txout)"
             ");"
        );
    }

    // Addresses
    if (table == "Addresses" || table == "ALL") {
        exec("create table if not exists Addresses ("
             "address text primary key not null,"
             "txid text not null,"
             "block text not null,"
             "time int not null"
             ");"
        );
    }

    // Comment
    if (table == "Comment" || table == "ALL") {
        exec("create table if not exists Comment ("
             "txid text primary key not null,"
             "otxid text not null,"
             "last int not null,"
             "postid text not null,"
             "address text not null,"
             "time int not null,"
             "block int not null,"
             "msg text not null,"
             "parentid text"
             "answerid text"
             "scoreUp int"
             "scoreDown int"
             "reputation int"
             ");"
        );
    }

    // CommentRatings
    if (table == "CommentRatings" || table == "ALL") {
        exec("create table if not exists CommentRatings ("
             "block int not null,"
             "commentid text not null,"
             "scoreUp text not null,"
             "scoreDown int not null,"
             "reputation int not null,"
             "primary key (commentid, block)"
             ");"
        );
    }

    // CommentScores
    if (table == "CommentScores" || table == "ALL") {
        exec("create table if not exists CommentScores ("
             "txid text primary key not null,"
             "block int not null,"
             "time int not null,"
             "commentid text not null,"
             "address text not null,"
             "value int not null"
             ");"
        );
    }

    return true;
}

// ---------------------- ADD -------------------------

bool PocketRepository::Add(const Utxo &itm) {
    return exec(
        tfm::format(
            " insert into Utxo ("
            "   txid,"
            "   txout,"
            "   time,"
            "   block,"
            "   address,"
            "   amount,"
            "   spent_block"
            " ) values ('%s',%d,%ld,%d,'%s',%ld,%d);",
            sql(itm.Txid), itm.Txout, itm.Time, itm.Block, sql(itm.Address), itm.Amount, itm.SpentBlock
        )
    );
}

bool PocketRepository::Add(const User &itm) {
    return exec(
        tfm::format(
            " insert into Users ("
            "  Address,"
            "  Id,"
            "  Txid,"
            "  Block,"
            "  Time,"
            "  Name,"
            "  Birthday,"
            "  Gender,"
            "  RegDate,"
            "  Avatar,"
            "  About,"
            "  Lang,"
            "  Url,"
            "  Pubkey,"
            "  Donations,"
            "  Referrer"
            " ) values ('%s',%d,'%s',%d,%ld,'%s',%d,%d,%ld,'%s','%s','%s','%s','%s','%s','%s');",
            sql(itm.Address), itm.Id, itm.Txid, itm.Block, itm.Time, sql(itm.Name),
            itm.Birthday, itm.Gender, itm.RegDate, sql(itm.Avatar), sql(itm.About), sql(itm.Lang),
            sql(itm.Url), sql(itm.Pubkey), sql(itm.Donations), sql(itm.Referrer)
        )
    );
}

// TODO (brangr): complete all functions
//bool PocketRepository::Add(const Post& itm) {
//    return exec(
//        tfm::format(
//            "insert into Posts ( \
//                Txid, \
//                TxidEdit, \
//                TxidRepost, \
//                Block, \
//                Time, \
//                Address, \
//                Type, \
//                Lang, \
//                Caption, \
//                Message, \
//                Tags, \
//                Url, \
//                Settings \
//            ) values ('%s',%d,'%s',%d,%ld,'%s',%d,%d,%ld,'%s','%s','%s','%s','%s','%s','%s');",
//            itm.Txid, itm.TxidEdit, itm.TxidRepost, itm.Block,
//
//            sql(itm.Address), itm.Id, , itm.Block, itm.Time, sql(itm.Name),
//            itm.Birthday, itm.Gender, itm.RegDate, sql(itm.Avatar), sql(itm.About), sql(itm.Lang),
//            sql(itm.Url), sql(itm.Pubkey), sql(itm.Donations), sql(itm.Referrer)
//        )
//    );
//}

bool PocketRepository::Add(Checkpoint itm) {
    return exec(
        tfm::format(
            " insert into Benchmark ("
            "  Begin,"
            "  End,"
            "  Checkpoint,"
            "  Payload"
            " ) values (%lld,%lld,'%s','%s');",
            itm.Begin, itm.End, sql(itm.Checkpoint), sql(itm.Payload)
        )
    );
}


//-----------------------------------------------------

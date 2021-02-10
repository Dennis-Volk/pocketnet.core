// Copyright (c) 2021 PocketNet developers
// PocketDB general wrapper
//-----------------------------------------------------
#include "pocketrepository.h"
#include <tinyformat.h>

//-----------------------------------------------------

std::unique_ptr<PocketRepository> g_pocket_repository;

PocketRepository::PocketRepository()
{
    if (sqlite3_open((GetDataDir() / "pocketdb" / "main.sqlite3").string().c_str(), &db)) {
        LogPrintf("Cannot open Sqlite DB (%s) - %s\n", (GetDataDir() / "pocketdb" / "general.sqlite3").string(),
            sqlite3_errmsg(db));
        sqlite3_close(db);
    }
}

PocketRepository::~PocketRepository()
{
    sqlite3_close(db);
}

//-----------------------------------------------------
bool PocketRepository::exec(const string& sql)
{
    char* zErrMsg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &zErrMsg)) {
        // todo log
        sqlite3_free(zErrMsg);
        return false;
    }

    return true;
}
//-----------------------------------------------------

bool PocketRepository::Init(const string& table)
{
    // Service
    if (table == "Service" || table == "ALL") {
        exec(" create table if not exists Service ("
             "  Service int primary key not null"
             " );");
    }

    // CommentScores
    if (table == "Benchmark" || table == "ALL") {
        exec(" create table if not exists Benchmark ("
             "  Begin int not null,"
             "  End int not null,"
             "  Checkpoint text not null,"
             "  Payload text not null"
             " );");
    }

    // Pocket Mempool
    if (table == "Mempool" || table == "ALL") {
        exec(" create table if not exists Mempool ("
             "  Txid text primary key not null,"
             "  TxidSource text,"
             "  Model text not null,"
             "  Data blob not null"
             " );");
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
             " );");
    }

    // UserRatings
    if (table == "UserRatings" || table == "ALL") {
        exec(" create table if not exists UserRatings ("
             "  Block int not null,"
             "  Address text not null,"
             "  Reputation int not null,"
             " primary key (Address, Block)"
             " );");
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
             " );");
    }

    // PostRatings
    if (table == "PostRatings" || table == "ALL") {
        exec("create table if not exists PostRatings ("
             " Block int not null,"
             " PostTxid text not null,"
             " ScoreSum text not null,"
             " ScoreCnt int not null,"
             " Reputation int not null,"
             " primary key (PostTxid, Block)"
             ");");
    }

    // Scores
    if (table == "Scores" || table == "ALL") {
        exec("create table if not exists Scores ("
             " Txid text primary key not null,"
             " Block int not null,"
             " Time int not null,"
             " PostTxid text not null,"
             " Address text not null,"
             " Value int not null"
             ");");
    }

    // Subscribes
    if (table == "Subscribes" || table == "ALL") {
        exec("create table if not exists Subscribes ("
             " Txid text primary key not null,"
             " Block int not null,"
             " Time int not null,"
             " Address text not null,"
             " Address_to text not null,"
             " Private int not null,"
             " Unsubscribe int not null"
             ");");
    }

    // Blockings
    if (table == "Blockings" || table == "ALL") {
        exec("create table if not exists Blocking ("
             " Txid text primary key not null,"
             " Block int not null,"
             " Time int not null,"
             " Address text not null,"
             " Address_to text not null,"
             " Unblocking int not null"
             " AddressReputation int not null"
             ");");
    }

    // Complains
    if (table == "Complains" || table == "ALL") {
        exec("create table if not exists Complains ("
             " Txid text primary key not null,"
             " Block int not null,"
             " Time int not null,"
             " Posttxid text not null,"
             " Address text not null,"
             " Reason int not null"
             ");");
    }

    // UTXO
    if (table == "Utxo" || table == "ALL") {
        exec("create table if not exists Utxo ("
             " Txid text not null,"
             " Txout int not null,"
             " Time int not null,"
             " Block int not null,"
             " Address text not null,"
             " Amount int not null,"
             " SpentBlock int,"
             " primary key (Txid, Txout)"
             ");");
    }

    // Addresses
    if (table == "Addresses" || table == "ALL") {
        exec("create table if not exists Addresses ("
             " Address text primary key not null,"
             " Txid text not null,"
             " Block text not null,"
             " Time int not null"
             ");");
    }

    // Comment
    if (table == "Comment" || table == "ALL") {
        exec("create table if not exists Comment ("
             " Txid text primary key not null,"
             " TxidEdit text not null,"
             " PostTxid text not null,"
             " Address text not null,"
             " Time int not null,"
             " Block int not null,"
             " Msg text not null,"
             " ParentTxid text"
             " AnswerTxid text"
             " ScoreUp int"
             " ScoreDown int"
             " Reputation int"
             ");");
    }

    // CommentRatings
    if (table == "CommentRatings" || table == "ALL") {
        exec("create table if not exists CommentRatings ("
             " Block int not null,"
             " CommentTxid text not null,"
             " ScoreUp text not null,"
             " ScoreDown int not null,"
             " Reputation int not null,"
             " primary key (CommentTxid, Block)"
             ");");
    }

    // CommentScores
    if (table == "CommentScores" || table == "ALL") {
        exec("create table if not exists CommentScores ("
             " Txid text primary key not null,"
             " Block int not null,"
             " Time int not null,"
             " CommentTxid text not null,"
             " Address text not null,"
             " Value int not null"
             ");");
    }

    return true;
}

// ---------------------- ADD -------------------------

//bool PocketRepository::Add(const Utxo &itm) {
//    return exec(
//        tfm::format(
//            " insert into Utxo ("
//            "   txid,"
//            "   txout,"
//            "   time,"
//            "   block,"
//            "   address,"
//            "   amount,"
//            "   spent_block"
//            " ) values ('%s',%d,%ld,%d,'%s',%ld,%d);",
//            sql(itm.Txid), itm.Txout, itm.Time, itm.Block, sql(itm.Address), itm.Amount, itm.SpentBlock
//        )
//    );
//}
//
//bool PocketRepository::Add(const User &itm) {
//    return exec(
//        tfm::format(
//            " insert into Users ("
//            "  Address,"
//            "  Id,"
//            "  Txid,"
//            "  Block,"
//            "  Time,"
//            "  Name,"
//            "  Birthday,"
//            "  Gender,"
//            "  RegDate,"
//            "  Avatar,"
//            "  About,"
//            "  Lang,"
//            "  Url,"
//            "  Pubkey,"
//            "  Donations,"
//            "  Referrer"
//            " ) values ('%s',%d,'%s',%d,%ld,'%s',%d,%d,%ld,'%s','%s','%s','%s','%s','%s','%s');",
//            sql(itm.Address), itm.Id, itm.Txid, itm.Block, itm.Time, sql(itm.Name),
//            itm.Birthday, itm.Gender, itm.RegDate, sql(itm.Avatar), sql(itm.About), sql(itm.Lang),
//            sql(itm.Url), sql(itm.Pubkey), sql(itm.Donations), sql(itm.Referrer)
//        )
//    );
//}
//
//// TODO (brangr): complete all functions
////bool PocketRepository::Add(const Post& itm) {
////    return exec(
////
////        )
////    );
////}

bool PocketRepository::Add(PocketModel* itm)
{
    //return exec(itm->SqlInsert());
}


PocketModel* PocketRepository::GetCachedTransaction(uint256 hash)
{
    LOCK(cs);
    if (pocketDataCache.find(hash) != pocketDataCache.end())
        return pocketDataCache[hash];

    return nullptr;
}

void PocketRepository::AddCachedTransaction(PocketModel* itm)
{
    LOCK(cs);
    if (pocketDataCache.find(itm->TxId()) != pocketDataCache.end())
        pocketDataCache.emplace(itm->TxId(), itm);
}

void PocketRepository::AddCachedTransactions(vector<PocketModel*> itms)
{
    for (auto it : itms) {
        AddCachedTransaction(it);
    }
}

void PocketRepository::RemoveCachedTransaction(uint256 hash)
{
    LOCK(cs);
    if (pocketDataCache.find(hash) != pocketDataCache.end())
        pocketDataCache.erase(hash);
}


//-----------------------------------------------------

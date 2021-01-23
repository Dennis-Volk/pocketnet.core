// Copyright (c) 2021 PocketNet developers
// PocketDB general wrapper
//-----------------------------------------------------
#include "sqlite.h"
//-----------------------------------------------------
SqliteRepository::SqliteRepository(sqlite3* sqLiteDb) {
    db = sqLiteDb;
}

SqliteRepository::~SqliteRepository() {}
//-----------------------------------------------------
bool SqliteRepository::_exec(string sql) {
    char *zErrMsg = 0;
    if (sqlite3_exec(db, sql.c_str(), 0, 0, &zErrMsg)) {
        // todo log
        sqlite3_free(zErrMsg);
        return false;
    }
    
    return true;
}
//-----------------------------------------------------
bool SqliteRepository::Add(Utxo utxo) {
    string sql = "insert into Utxo (txid, txout, time, block, address, amount, spent_block) values ( '" + \
            utxo.txid + "'," + \
        to_string(utxo.txout) + "," + \
        to_string(utxo.time) + "," + \
        to_string(utxo.block) + ",'" + \
            utxo.address + "'," + \
        to_string(utxo.amount) + "," + \
        to_string(utxo.spent_block) + \
        ");";

    return _exec(sql);
}
//-----------------------------------------------------

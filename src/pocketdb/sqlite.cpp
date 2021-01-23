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
bool SqliteRepository::_exec(char* sql) {
    char *zErrMsg = 0;
    if (sqlite3_exec(db, sql, NULL, 0, &zErrMsg)) {
        // todo log
        sqlite3_free(zErrMsg);
        return false;
    }
    
    return true;
}
//-----------------------------------------------------
bool SqliteRepository::Add(Utxo utxo) {
    char* sql;
    
    printf(sql,
        "insert into Utxo (txid, txout, time, block, address, amount, spent_block) values ('%s', %d, %ld, '%d', '%s', %ld, %d);",
        utxo.txid,
        utxo.txout,
        utxo.time,
        utxo.block,
        utxo.address,
        utxo.amount,
        utxo.spent_block
    );

    return _exec(sql);
}
//-----------------------------------------------------

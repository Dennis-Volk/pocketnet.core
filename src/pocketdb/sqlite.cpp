// Copyright (c) 2021 PocketNet developers
// PocketDB general wrapper
//-----------------------------------------------------
#include "sqlite.h"
#include <tinyformat.h>
//-----------------------------------------------------
SqliteRepository::SqliteRepository(sqlite3* sqLiteDb) {
    db = sqLiteDb;
}

SqliteRepository::~SqliteRepository() {}
//-----------------------------------------------------
bool SqliteRepository::exec(string sql) {
    char *zErrMsg = 0;
    if (sqlite3_exec(db, sql.c_str(), 0, 0, &zErrMsg)) {
        // todo log
        sqlite3_free(zErrMsg);
        return false;
    }
    
    return true;
}
//-----------------------------------------------------
// ADD

bool SqliteRepository::Add(Utxo utxo) {
    return exec(
        tfm::format(
            "insert into Utxo (txid, txout, time, block, address, amount, spent_block) values ('%s',%d,%ld,%d,'%s',%ld,%d);",
            sql(utxo.Txid), utxo.Txout, utxo.Time, utxo.Block, sql(utxo.Address), utxo.Amount, utxo.SpentBlock
        )
    );
}

bool SqliteRepository::Add(UserView user) {
    return exec(
        tfm::format(
            "insert into Utxo (txid, txout, time, block, address, amount, spent_block) values ('%s',%d,%ld,%d,'%s',%ld,%d);",
            sql(utxo.Txid), utxo.Txout, utxo.Time, utxo.Block, sql(utxo.Address), utxo.Amount, utxo.SpentBlock
        )
    );
}
//-----------------------------------------------------

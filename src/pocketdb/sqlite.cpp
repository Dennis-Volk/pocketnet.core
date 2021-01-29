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
// ---------------------- ADD -------------------------

bool SqliteRepository::Add(Utxo itm) {
    return exec(
        tfm::format(
            "insert into Utxo ( \
                txid, \
                txout, \
                time, \
                block, \
                address, \
                amount, \
                spent_block \
            ) values ('%s',%d,%ld,%d,'%s',%ld,%d);",
            sql(itm.Txid), itm.Txout, itm.Time, itm.Block, sql(itm.Address), itm.Amount, itm.SpentBlock
        )
    );
}

bool SqliteRepository::Add(User itm) {
    return exec(
        tfm::format(
            "insert into Users ( \
                Address, \
                Id, \
                Txid, \
                Block, \
                Time, \
                Name, \
                Birthday, \
                Gender, \
                RegDate, \
                Avatar, \
                About, \
                Lang, \
                Url, \
                Pubkey, \
                Donations, \
                Referrer \
            ) values ('%s',%d,'%s',%d,%ld,'%s',%d,%d,%ld,'%s','%s','%s','%s','%s','%s','%s');",
            sql(itm.Address), itm.Id, itm.Txid, itm.Block, itm.Time, sql(itm.Name),
            itm.Birthday, itm.Gender, itm.RegDate, sql(itm.Avatar), sql(itm.About), sql(itm.Lang),
            sql(itm.Url), sql(itm.Pubkey), sql(itm.Donations), sql(itm.Referrer)
        )
    );
}

bool SqliteRepository::Add(Post itm) {
    return exec(
        tfm::format(
            "insert into Posts ( \
                Txid, \
                TxidEdit, \
                TxidRepost, \
                Block, \
                Time, \
                Address, \
                Type, \
                Lang, \
                Caption, \
                Message, \
                Tags, \
                Url, \
                Settings \
            ) values ('%s',%d,'%s',%d,%ld,'%s',%d,%d,%ld,'%s','%s','%s','%s','%s','%s','%s');",
            itm.Txid, itm.TxidEdit, itm.TxidRepost, itm.Block, 

            sql(itm.Address), itm.Id, , itm.Block, itm.Time, sql(itm.Name),
            itm.Birthday, itm.Gender, itm.RegDate, sql(itm.Avatar), sql(itm.About), sql(itm.Lang),
            sql(itm.Url), sql(itm.Pubkey), sql(itm.Donations), sql(itm.Referrer)
        )
    );
}






















//-----------------------------------------------------

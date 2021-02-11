#pragma once

#include <uint256.h>
#include <map>
#include "pocketdb/pocketmodels.h"

class PocketBlock {
private:


public:

    PocketBlock();
    PocketBlock(std::vector<PocketModel*>& vtx);
    ~PocketBlock();

    std::vector<PocketModel*> Transactions;

    void Add(PocketModel* itm);
    bool Find(uint256 txid, PocketModel* itm);
    void SetBlockHeight(int height);
};
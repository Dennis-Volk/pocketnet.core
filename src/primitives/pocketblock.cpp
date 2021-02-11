#include "pocketblock.h"

PocketBlock::PocketBlock()
{
}

PocketBlock::~PocketBlock()
{
}

PocketBlock::PocketBlock(std::vector<PocketModel*>& vtx)
{
    Transactions = vtx;
}

void PocketBlock::Add(PocketModel* itm)
{
    Transactions.push_back(itm);
}

bool PocketBlock::Find(uint256 txid, PocketModel* itm)
{
    auto res = std::find_if(std::begin(Transactions), std::end(Transactions), [txid](PocketModel* it) { return it->TxId() == txid; });

    if (res != std::end(Transactions)) {
        itm = *res;
        return true;
    }

    return false;
}

void PocketBlock::SetBlockHeight(int height)
{
    for (PocketModel* it : Transactions) {
        it->Block = height;
    }
}

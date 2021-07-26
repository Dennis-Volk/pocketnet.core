// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_EXPLORERREPOSITORY_H
#define POCKETDB_EXPLORERREPOSITORY_H

#include "pocketdb/helpers/TransactionHelper.hpp"
#include "pocketdb/repositories/BaseRepository.hpp"

namespace PocketDb
{
    using namespace std;
    using namespace PocketTx;
    using namespace PocketHelpers;

    class ExplorerRepository : public BaseRepository
    {
    public:
        explicit ExplorerRepository(SQLiteDatabase& db) : BaseRepository(db) {}

        void Init() override;
        void Destroy() override;

        map<PocketTxType, map<int, int>> GetStatistic(int lastHeight, int count);
        map<PocketTxType, int> GetStatistic(int64_t startTime, int64_t endTime);
        tuple<int64_t, int64_t> GetAddressSpent(const string& addressHash);
        UniValue GetAddressTransactions(const string& addressHash, int firstNumber);

    private:
    
    };

} // namespace PocketDb

#endif // POCKETDB_EXPLORERREPOSITORY_H
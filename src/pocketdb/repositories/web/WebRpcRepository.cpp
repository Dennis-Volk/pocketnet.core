// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/WebRpcRepository.h"

namespace PocketDb
{
    void WebRpcRepository::Init() {}

    void WebRpcRepository::Destroy() {}

    UniValue WebRpcRepository::GetAddressId(const string& address)
    {
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(
            SELECT String1, Id
            FROM Transactions
            WHERE Type in (100, 101, 102)
              and Height is not null
              and Last = 1
              and String1 = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) result.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 1); ok) result.pushKV("id", value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetAddressId(int64_t id)
    {
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(
            SELECT String1, Id
            FROM Transactions
            WHERE Type in (100, 101, 102)
              and Height is not null
              and Last = 1
              and Id = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementInt64(stmt, 1, id);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) result.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 1); ok) result.pushKV("id", value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetUserAddress(const string& name)
    {
        UniValue result(UniValue::VARR);

        auto _name = EscapeValue(name);

        string sql = R"sql(
            select p.String2, u.String1
            from Payload p indexed by Payload_String2_nocase_TxHash
            cross join Transactions u indexed by Transactions_Hash_Height
                on u.Type in (100, 101, 102) and u.Height > 0 and u.Hash = p.TxHash and u.Last = 1
            where p.String2 like ? escape '\'
            limit 1
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, _name);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("name", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("address", valueStr);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetAddressesRegistrationDates(const vector<string>& addresses)
    {
        auto result = UniValue(UniValue::VARR);

        if (addresses.empty())
            return result;

        string sql = R"sql(
            select u.String1, u.Time, u.Hash
            from Transactions u indexed by Transactions_Type_Last_String1_Height_Id
            where u.Type in (100, 101, 102)
            and u.Last in (0,1)
            and u.String1 in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
            and u.Height = (
                select min(uf.Height)
                from Transactions uf indexed by Transactions_Id
                where uf.Id = u.Id
            )
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            for (size_t i = 0; i < addresses.size(); i++)
                TryBindStatementText(stmt, (int) i + 1, addresses[i]);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("address", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("time", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("txid", valueStr);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetTopAddresses(int count)
    {
        UniValue result(UniValue::VARR);

        auto sql = R"sql(
            select AddressHash, Value
            from Balances indexed by Balances_Last_Value
            where Last = 1
            order by Value desc
            limit ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementInt(stmt, 1, count);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue addr(UniValue::VOBJ);
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) addr.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 1); ok) addr.pushKV("balance", value);
                result.push_back(addr);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetAccountState(const string& address, int heightWindow)
    {
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(
            select
                u.String1 as Address,
                up.String2 as Name,

                (select reg.Time from Transactions reg indexed by Transactions_Id
                    where reg.Id=u.Id and reg.Height=(select min(reg1.Height) from Transactions reg1 indexed by Transactions_Id where reg1.Id=reg.Id)) as RegistrationDate,

                ifnull((select r.Value from Ratings r indexed by Ratings_Type_Id_Last_Height
                    where r.Type=0 and r.Id=u.Id and r.Last=1),0) as Reputation,

                ifnull((select b.Value from Balances b indexed by Balances_AddressHash_Last
                    where b.AddressHash=u.String1 and b.Last=1),0) as Balance,

                (select count(1) from Ratings r indexed by Ratings_Type_Id_Last_Height
                    where r.Type=1 and r.Id=u.Id) as Likers,

                (select count(1) from Transactions p indexed by Transactions_Type_String1_Height_Time_Int1
                    where p.Type in (200) and p.Hash=p.String2 and p.String1=u.String1 and p.Height>=?) as PostSpent,

                (select count(1) from Transactions p indexed by Transactions_Type_String1_Height_Time_Int1
                    where p.Type in (201) and p.Hash=p.String2 and p.String1=u.String1 and p.Height>=?) as VideoSpent,

                (select count(1) from Transactions p indexed by Transactions_Type_String1_Height_Time_Int1
                    where p.Type in (204) and p.String1=u.String1 and p.Height>=?) as CommentSpent,

                (select count(1) from Transactions p indexed by Transactions_Type_String1_Height_Time_Int1
                    where p.Type in (300) and p.String1=u.String1 and p.Height>=?) as ScoreSpent,

                (select count(1) from Transactions p indexed by Transactions_Type_String1_Height_Time_Int1
                    where p.Type in (301) and p.String1=u.String1 and p.Height>=?) as ScoreCommentSpent,

                (select count(1) from Transactions p indexed by Transactions_Type_String1_Height_Time_Int1
                    where p.Type in (307) and p.String1=u.String1 and p.Height>=?) as ComplainSpent

            from Transactions u indexed by Transactions_Type_Last_String1_Height_Id
            join Payload up on up.TxHash=u.Hash

            where u.Type in (100, 101, 102)
            and u.Height is not null
            and u.String1 = ?
            and u.Last = 1
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementInt(stmt, 1, heightWindow);
            TryBindStatementInt(stmt, 2, heightWindow);
            TryBindStatementInt(stmt, 3, heightWindow);
            TryBindStatementInt(stmt, 4, heightWindow);
            TryBindStatementInt(stmt, 5, heightWindow);
            TryBindStatementInt(stmt, 6, heightWindow);
            TryBindStatementText(stmt, 7, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) result.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) result.pushKV("name", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 2); ok) result.pushKV("user_reg_date", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 3); ok) result.pushKV("reputation", value / 10);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 4); ok) result.pushKV("balance", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 5); ok) result.pushKV("likers", value);

                if (auto[ok, value] = TryGetColumnInt64(*stmt, 6); ok) result.pushKV("post_spent", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 7); ok) result.pushKV("video_spent", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 8); ok) result.pushKV("comment_spent", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 9); ok) result.pushKV("score_spent", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 10); ok) result.pushKV("comment_score_spent", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 11); ok) result.pushKV("complain_spent", value);

                // ??
                // result.pushKV("number_of_blocking", number_of_blocking);
                // result.pushKV("addr_reg_date", address_registration_date);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetAccountSetting(const string& address)
    {
        string result;

        string sql = R"sql(
            select p.String1
            from Transactions t indexed by Transactions_Type_Last_String1_Height_Id
            join Payload p on p.TxHash = t.Hash
            where t.Type in (103)
              and t.Last = 1
              and t.Height is not null
              and t.String1 = ?
            limit 1
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, address);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok)
                    result = value;
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetUserStatistic(const vector<string>& addresses, const int nHeight, const int depth)
    {
        UniValue result(UniValue::VARR);

        if (addresses.empty())
            return  result;

        string addressesWhere = join(vector<string>(addresses.size(), "?"), ",");

        string sql = R"sql(
            select
                u.String1 as Address,
                ifnull((select count(distinct ru.String1) from Transactions ru indexed by Transactions_Type_Last_String2_Height
                    where ru.Type in (100,101,102) and ru.Last=1 and ru.Height <= ? and ru.Height > ? and ru.String2=u.String1),0) as ReferralsCountHist
            from Transactions u indexed by Transactions_Type_Last_String1_Height_Id
            where u.Type in (100, 102, 102)
            and u.Height is not null
            and u.String1 in ( )sql" + addressesWhere + R"sql( )
            and u.Last = 1
            group by u.String1
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            TryBindStatementInt(stmt, i++, nHeight);
            TryBindStatementInt(stmt, i++, nHeight - depth);
            for (const auto& address: addresses)
                TryBindStatementText(stmt, i++, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);
                auto[ok0, address] = TryGetColumnString(*stmt, 0);
                auto[ok1, ReferralsCountHist] = TryGetColumnInt(*stmt, 1);

                record.pushKV("address", address);
                record.pushKV("histreferals", ReferralsCountHist);
                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    vector<tuple<string, int64_t, UniValue>> WebRpcRepository::GetAccountProfiles(const vector<string>& addresses, const vector<int64_t>& ids, bool shortForm)
    {
        vector<tuple<string, int64_t, UniValue>> result{};

        if (addresses.empty() && ids.empty())
            return result;
        
        string where;
        if (!addresses.empty())
            where += " and u.String1 in (" + join(vector<string>(addresses.size(), "?"), ",") + ") ";
        if (!ids.empty())
            where += " and u.Id in (" + join(vector<string>(ids.size(), "?"), ",") + ") ";

        string fullProfileSql = "";
        if (!shortForm)
        {
            fullProfileSql = R"sql(

                , (
                    select json_group_array(json_object('address', subs.String1, 'private', case when subs.Type == 303 then '1' else '0' end))
                    from Transactions subs indexed by Transactions_Type_Last_String1_Height_Id
                    where subs.Type in (302,303) and subs.Height is not null and subs.Last = 1 and subs.String1 = u.String1
                ) as Subscribes
                
                , (
                    select json_group_array(subs.String1)
                    from Transactions subs indexed by Transactions_Type_Last_String2_Height
                    where subs.Type in (302,303) and subs.Height is not null and subs.Last = 1 and subs.String2 = u.String1
                ) as Subscribers

                , (
                    select json_group_array(blck.String2)
                    from Transactions blck indexed by Transactions_Type_Last_String1_Height_Id
                    where blck.Type in (305) and blck.Height is not null and blck.Last = 1 and blck.String1 = u.String1
                ) as Blockings

            )sql";
        }

        string sql = R"sql(
            select
                  u.String1 as Address
                , u.Id
                , p.String2 as Name
                , p.String3 as Avatar
                , p.String7 as Donations
                , ifnull(u.String2,'') as Referrer

                , ifnull((
                    select count(1) from Transactions ru indexed by Transactions_Type_Last_String2_Height
                    where ru.Type in (100,101,102) and ru.Last=1 and ru.Height is not null and ru.String2=u.String1)
                ,0) as ReferralsCount

                , ifnull((
                    select count(1)
                    from Transactions po indexed by Transactions_Type_Last_String1_Height_Id
                    where po.Type in (200,201) and po.Last=1 and po.Height is not null and po.String1=u.String1)
                ,0) as PostsCount

                , ifnull((
                    select r.Value
                    from Ratings r indexed by Ratings_Type_Id_Last_Height
                    where r.Type=0 and r.Id=u.Id and r.Last=1)
                ,0) as Reputation

                , (
                    select count(*)
                    from Transactions subs indexed by Transactions_Type_Last_String1_Height_Id
                    where subs.Type in (302,303) and subs.Height is not null and subs.Last = 1 and subs.String1 = u.String1
                ) as SubscribesCount

                , (
                    select count(*)
                    from Transactions subs indexed by Transactions_Type_Last_String2_Height
                    where subs.Type in (302,303) and subs.Height is not null and subs.Last = 1 and subs.String2 = u.String1
                ) as SubscribersCount

                , (
                    select count(*)
                    from Transactions subs indexed by Transactions_Type_Last_String1_Height_Id
                    where subs.Type in (305) and subs.Height is not null and subs.Last = 1 and subs.String1 = u.String1
                ) as BlockingsCount

                , (
                    select count(*)
                    from Ratings lkr indexed by Ratings_Type_Id_Last_Height
                    where lkr.Type = 1 and lkr.Id = u.Id
                ) as Likers

                , p.String6 as Pubkey
                , p.String4 as About
                , p.String1 as Lang
                , p.String5 as Url
                , u.Time

                , (
                    select reg.Time
                    from Transactions reg indexed by Transactions_Id
                    where reg.Id=u.Id and reg.Height is not null order by reg.Height asc limit 1
                ) as RegistrationDate

                )sql" + fullProfileSql + R"sql(

            from Transactions u indexed by Transactions_Type_Last_String1_Height_Id
            join Payload p on p.TxHash=u.Hash

            where u.Type in (100,101,102)
              and u.Last=1
              and u.Height is not null
              )sql" + where + R"sql(
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            for (const string& address : addresses)
                TryBindStatementText(stmt, i++, address);
            for (int64_t id : ids)
                TryBindStatementInt64(stmt, i++, id);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto[ok0, address] = TryGetColumnString(*stmt, 0);
                auto[ok2, id] = TryGetColumnInt64(*stmt, 1);

                UniValue record(UniValue::VOBJ);

                record.pushKV("address", address);
                record.pushKV("id", id);
                if (IsDeveloper(address)) record.pushKV("dev", true);

                if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) record.pushKV("name", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok) record.pushKV("i", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("b", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) record.pushKV("r", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 6); ok) record.pushKV("rc", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 7); ok) record.pushKV("postcnt", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 8); ok) record.pushKV("reputation", value / 10.0);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 9); ok) record.pushKV("subscribes_count", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 10); ok) record.pushKV("subscribers_count", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 11); ok) record.pushKV("blockings_count", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 12); ok) record.pushKV("likers_count", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 13); ok) record.pushKV("k", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 14); ok) record.pushKV("a", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 15); ok) record.pushKV("l", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 16); ok) record.pushKV("s", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 17); ok) record.pushKV("update", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 18); ok) record.pushKV("regdate", value);

                if (!shortForm)
                {
                    
                    if (auto[ok, value] = TryGetColumnString(*stmt, 19); ok)
                    {
                        UniValue subscribes(UniValue::VARR);
                        subscribes.read(value);
                        record.pushKV("subscribes", subscribes);
                    }

                    if (auto[ok, value] = TryGetColumnString(*stmt, 20); ok)
                    {
                        UniValue subscribes(UniValue::VARR);
                        subscribes.read(value);
                        record.pushKV("subscribers", subscribes);
                    }

                    if (auto[ok, value] = TryGetColumnString(*stmt, 21); ok)
                    {
                        UniValue subscribes(UniValue::VARR);
                        subscribes.read(value);
                        record.pushKV("blocking", subscribes);
                    }
                }

                result.emplace_back(address, id, record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    map<string, UniValue> WebRpcRepository::GetAccountProfiles(const vector<string>& addresses, bool shortForm)
    {
        map<string, UniValue> result{};

        auto _result = GetAccountProfiles(addresses, {}, shortForm);
        for (const auto[address, id, record] : _result)
            result.insert_or_assign(address, record);

        return result;
    }

    map<int64_t, UniValue> WebRpcRepository::GetAccountProfiles(const vector<int64_t>& ids, bool shortForm)
    {
        map<int64_t, UniValue> result{};

        auto _result = GetAccountProfiles({}, ids, shortForm);
        for (const auto[address, id, record] : _result)
            result.insert_or_assign(id, record);

        return result;
    }

    UniValue WebRpcRepository::GetLastComments(int count, int height, const string& lang)
    {
        auto result = UniValue(UniValue::VARR);
        return result;
        
        // TODO (team): The method is no longer needed? 

        auto sql = R"sql(
            WITH RowIds AS (
                SELECT MAX(c.RowId) as RowId
                FROM Transactions c
                JOIN Payload pl ON pl.TxHash = c.Hash
                WHERE c.Type in (204,205)
                    and c.Last=1
                    and c.Height is not null
                    and c.Height <= ?
                    and c.Time < ?
                    and pl.String1 = ?
                GROUP BY c.Id
                )
            select t.Hash,
                t.String2 as RootTxHash,
                t.String3 as PostTxHash,
                t.String1 as AddressHash,
                t.Time,
                t.Height,
                t.String4 as ParentTxHash,
                t.String5 as AnswerTxHash,
                (select count(1) from Transactions sc where sc.Type=301 and sc.Height is not null and sc.String2=t.Hash and sc.Int1=1) as ScoreUp,
                (select count(1) from Transactions sc where sc.Type=301 and sc.Height is not null and sc.String2=t.Hash and sc.Int1=-1) as ScoreDown,
                (select r.Value from Ratings r where r.Id=t.Id and r.Type=3 and r.Last=1) as Reputation,
                pl.String2 AS Msg
            from Transactions t
            join RowIds rid on t.RowId = rid.RowId
            join Payload pl ON pl.TxHash = t.Hash
            where t.Type in (204,205) and t.Last=1 and t.height is not null
            order by t.Height desc, t.Time desc
            limit ?;
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementInt(stmt, 1, height);
            TryBindStatementInt64(stmt, 2, GetAdjustedTime());
            TryBindStatementText(stmt, 3, lang.empty() ? "en" : lang);
            TryBindStatementInt(stmt, 4, count);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                auto[ok0, txHash] = TryGetColumnString(*stmt, 0);
                auto[ok1, rootTxHash] = TryGetColumnString(*stmt, 1);

                record.pushKV("id", rootTxHash);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("postid", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 3); ok) record.pushKV("address", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 4); ok) record.pushKV("time", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 4); ok) record.pushKV("timeUpd", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 5); ok) record.pushKV("block", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 6); ok) record.pushKV("parentid", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 7); ok) record.pushKV("answerid", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 8); ok) record.pushKV("scoreUp", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 9); ok) record.pushKV("scoreDown", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 10); ok) record.pushKV("reputation", valueStr);

                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 11); ok)
                {
                    record.pushKV("msg", valueStr);
                    record.pushKV("deleted", false);
                }
                else
                {
                    record.pushKV("msg", "");
                    record.pushKV("deleted", true);
                }

                record.pushKV("edit", txHash != rootTxHash);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    map<int64_t, UniValue> WebRpcRepository::GetLastComments(const vector<int64_t>& ids, const string& address)
    {
        auto func = __func__;
        map<int64_t, UniValue> result;

        string sql = R"sql(
            select
                cmnt.contentId,
                cmnt.commentId,
                c.Type,
                c.String2 as RootTxHash,
                c.String3 as PostTxHash,
                c.String1 as AddressHash,
                (select corig.Time from Transactions corig where corig.Hash = c.String2)Time,
                c.Time as TimeUpdate,
                c.Height,
                (select p.String1 from Payload p where p.TxHash = c.Hash)Message,
                c.String4 as ParentTxHash,
                c.String5 as AnswerTxHash,

                (select count(1) from Transactions sc indexed by Transactions_Type_Last_String2_Height
                    where sc.Type=301 and sc.Last in (0,1) and sc.Height is not null and sc.String2 = c.Hash and sc.Int1 = 1) as ScoreUp,

                (select count(1) from Transactions sc indexed by Transactions_Type_Last_String2_Height
                    where sc.Type=301 and sc.Last in (0,1) and sc.Height is not null and sc.String2 = c.Hash and sc.Int1 = -1) as ScoreDown,

                (select r.Value from Ratings r indexed by Ratings_Type_Id_Last_Height
                    where r.Id = c.Id AND r.Type=3 and r.Last=1) as Reputation,

                (select count(*) from Transactions ch indexed by Transactions_Type_Last_String4_Height
                    where ch.Type in (204,205,206) and ch.Last = 1 and ch.Height is not null and ch.String4 = c.String2) as ChildrensCount,

                ifnull((select scr.Int1 from Transactions scr indexed by Transactions_Type_Last_String1_String2_Height
                    where scr.Type = 301 and scr.Last in (0,1) and scr.Height is not null and scr.String1 = ? and scr.String2 = c.String2),0) as MyScore,

                (
                    select o.Value
                    from TxOutputs o indexed by TxOutputs_TxHash_AddressHash_Value
                    where o.TxHash = c.Hash and o.AddressHash = cmnt.ContentAddressHash and o.AddressHash != c.String1
                ) as Donate,

                (
                    select count(1) from Transactions b  indexed by Transactions_Type_Last_String1_Height_Id
                    where b.Type in (305) and b.Last = 1 and b.Height is not null and b.String1 = cmnt.ContentAddressHash and b.String2 = c.String1
                )Blocked

            from (
                select

                    (t.Id)contentId,
                    (t.String1)ContentAddressHash,

                    -- Last comment for content record
                    (
                        select c1.Id
                        from Transactions c1 indexed by Transactions_Type_Last_String3_Height
                        left join TxOutputs o indexed by TxOutputs_TxHash_AddressHash_Value
                            on o.TxHash = c1.Hash and o.AddressHash = t.String1 and o.AddressHash != c1.String1
                        where c1.Type in (204, 205)
                          and c1.Last = 1
                          and c1.Height is not null
                          and c1.String3 = t.String2
                          and c1.String4 is null
                        order by o.Value desc, c1.Id desc
                        limit 1
                    )commentId

                from Transactions t indexed by Transactions_Last_Id_Height

                where t.Type in (200,201,207)
                    and t.Last = 1
                    and t.Height is not null
                    and t.Id in ( )sql" + join(vector<string>(ids.size(), "?"), ",") + R"sql( )

            ) cmnt

            join Transactions c indexed by Transactions_Last_Id_Height
                on c.Type in (204,205) and c.Last = 1 and c.Height is not null and c.Id = cmnt.commentId
        )sql";

        TryTransactionStep(func, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            int i = 1;

            TryBindStatementText(stmt, i++, address);

            for (int64_t id : ids)
                TryBindStatementInt64(stmt, i++, id);

            LogPrint(BCLog::SQL, "%s: %s\n", func, sqlite3_expanded_sql(*stmt));

            // ---------------------------
            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                auto[okContentId, contentId] = TryGetColumnInt(*stmt, 0);
                auto[okCommentId, commentId] = TryGetColumnInt(*stmt, 1);
                auto[okType, txType] = TryGetColumnInt(*stmt, 2);
                auto[okRoot, rootTxHash] = TryGetColumnString(*stmt, 3);

                record.pushKV("id", rootTxHash);
                record.pushKV("cid", commentId);
                record.pushKV("edit", (TxType)txType == CONTENT_COMMENT_EDIT);
                record.pushKV("deleted", (TxType)txType == CONTENT_COMMENT_DELETE);

                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("postid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) record.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 6); ok) record.pushKV("time", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 7); ok) record.pushKV("timeUpd", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 8); ok) record.pushKV("block", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 9); ok) record.pushKV("msg", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 10); ok) record.pushKV("parentid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 11); ok) record.pushKV("answerid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 12); ok) record.pushKV("scoreUp", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 13); ok) record.pushKV("scoreDown", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 14); ok) record.pushKV("reputation", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 15); ok) record.pushKV("children", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 16); ok) record.pushKV("myScore", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 17); ok)
                {
                    record.pushKV("donation", "true");
                    record.pushKV("amount", value);
                }
                if (auto[ok, value] = TryGetColumnInt(*stmt, 18); ok && value > 0) record.pushKV("blck", 1);
                                
                result.emplace(contentId, record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetCommentsByPost(const string& postHash, const string& parentHash, const string& addressHash)
    {
        auto func = __func__;
        auto result = UniValue(UniValue::VARR);

        string parentWhere = " and c.String4 is null ";
        if (!parentHash.empty())
            parentWhere = " and c.String4 = ? ";

        auto sql = R"sql(
            select

                c.Type,
                c.Hash,
                c.String2 as RootTxHash,
                c.String3 as PostTxHash,
                c.String1 as AddressHash,
                r.Time AS RootTime,
                c.Time,
                c.Height,
                pl.String1 AS Msg,
                c.String4 as ParentTxHash,
                c.String5 as AnswerTxHash,

                (select count(1) from Transactions sc indexed by Transactions_Type_Last_String2_Height
                    where sc.Type=301 and sc.Height is not null and sc.String2 = c.Hash and sc.Int1 = 1 and sc.Last in (0,1)) as ScoreUp,

                (select count(1) from Transactions sc indexed by Transactions_Type_Last_String2_Height
                    where sc.Type=301 and sc.Height is not null and sc.String2 = c.Hash and sc.Int1 = -1 and sc.Last in (0,1)) as ScoreDown,

                (select r.Value from Ratings r indexed by Ratings_Type_Id_Last_Height
                    where r.Id=c.Id and r.Type=3 and r.Last=1) as Reputation,

                sc.Int1 as MyScore,

                (select count(1) from Transactions s indexed by Transactions_Type_Last_String4_Height
                    where s.Type in (204, 205) and s.Height is not null and s.String4 = c.String2 and s.Last = 1) AS ChildrenCount,

                o.Value as Donate,

                (
                    select 1 from Transactions b  indexed by Transactions_Type_Last_String1_Height_Id
                    where b.Type in (305) and b.Last = 1 and b.Height is not null and b.String1 = t.String1 and b.String2 = c.String1
                    limit 1
                )Blocked

            from Transactions c indexed by Transactions_Type_Last_String3_Height

            join Transactions r ON c.String2 = r.Hash

            join Payload pl ON pl.TxHash = c.Hash

            join Transactions t indexed by Transactions_Type_Last_String2_Height
                on t.Type in (200,201) and t.Last = 1 and t.Height is not null and t.String2 = c.String3

            left join Transactions sc indexed by Transactions_Type_String1_String2_Height
                on sc.Type in (301) and sc.Height is not null and sc.String2 = c.String2 and sc.String1 = ?

            left join TxOutputs o indexed by TxOutputs_TxHash_AddressHash_Value
                on o.TxHash = c.Hash and o.AddressHash = t.String1 and o.AddressHash != c.String1

            where c.Type in (204, 205, 206)
                and c.Height is not null
                and c.Last = 1
                and c.String3 = ?
                )sql" + parentWhere + R"sql(

        )sql";

        TryTransactionStep(func, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            int i = 1;
            TryBindStatementText(stmt, i++, addressHash);
            TryBindStatementText(stmt, i++, postHash);
            if (!parentHash.empty())
                TryBindStatementText(stmt, i++, parentHash);

            LogPrint(BCLog::SQL, "%s: %s\n", func, sqlite3_expanded_sql(*stmt));

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                //auto[ok0, txHash] = TryGetColumnString(stmt, 1);
                auto[ok1, rootTxHash] = TryGetColumnString(*stmt, 2);
                record.pushKV("id", rootTxHash);

                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok)
                    record.pushKV("postid", value);

                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) record.pushKV("time", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 6); ok) record.pushKV("timeUpd", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 7); ok) record.pushKV("block", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 8); ok) record.pushKV("msg", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 9); ok) record.pushKV("parentid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 10); ok) record.pushKV("answerid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 11); ok) record.pushKV("scoreUp", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 12); ok) record.pushKV("scoreDown", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 13); ok) record.pushKV("reputation", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 14); ok) record.pushKV("myScore", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 15); ok) record.pushKV("children", value);

                if (auto[ok, value] = TryGetColumnString(*stmt, 16); ok)
                {
                    record.pushKV("amount", value);
                    record.pushKV("donation", "true");
                }

                if (auto[ok, value] = TryGetColumnInt(*stmt, 17); ok && value > 0) record.pushKV("blck", 1);

                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                {
                    switch (static_cast<TxType>(value))
                    {
                        case PocketTx::CONTENT_COMMENT:
                            record.pushKV("deleted", false);
                            record.pushKV("edit", false);
                            break;
                        case PocketTx::CONTENT_COMMENT_EDIT:
                            record.pushKV("deleted", false);
                            record.pushKV("edit", true);
                            break;
                        case PocketTx::CONTENT_COMMENT_DELETE:
                            record.pushKV("deleted", true);
                            record.pushKV("edit", true);
                            break;
                        default:
                            break;
                    }
                }

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetPagesScores(const vector<string>& postHashes, const vector<string>& commentHashes, const string& addressHash)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (!postHashes.empty())
        {
            string sql = R"sql(
                select
                    sc.String2 as ContentTxHash,
                    sc.Int1 as MyScoreValue

                from Transactions sc indexed by Transactions_Type_Last_String1_String2_Height

                where sc.Type in (300)
                  and sc.Last in (0,1)
                  and sc.Height is not null
                  and sc.String1 = ?
                  and sc.String2 in ( )sql" + join(vector<string>(postHashes.size(), "?"), ",") + R"sql( )
            )sql";

            TryTransactionStep(func, [&]()
            {
                auto stmt = SetupSqlStatement(sql);

                int i = 1;
                TryBindStatementText(stmt, i++, addressHash);
                for (const auto& postHash: postHashes)
                    TryBindStatementText(stmt, i++, postHash);

                LogPrint(BCLog::SQL, "%s: %s\n", func, sqlite3_expanded_sql(*stmt));

                while (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    UniValue record(UniValue::VOBJ);

                    if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("posttxid", value);
                    if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) record.pushKV("value", value);

                    result.push_back(record);
                }

                FinalizeSqlStatement(*stmt);
            });
        }

        if (!commentHashes.empty())
        {
            string commentHashesWhere;
            if (!commentHashes.empty())
                commentHashesWhere = " and c.String2 in (" + join(vector<string>(commentHashes.size(), "?"), ",") + ")";

            string sql = R"sql(
                select
                    c.String2 as RootTxHash,

                    (select count(1) from Transactions sc indexed by Transactions_Type_Last_String2_Height
                        where sc.Type in (301) and sc.Last in (0,1) and sc.Height is not null and sc.String2 = c.Hash and sc.Int1 = 1) as ScoreUp,

                    (select count(1) from Transactions sc indexed by Transactions_Type_Last_String2_Height
                        where sc.Type in (301) and sc.Last in (0,1) and sc.Height is not null and sc.String2 = c.Hash and sc.Int1 = -1) as ScoreDown,

                    (select r.Value from Ratings r indexed by Ratings_Type_Id_Last_Value where r.Id=c.Id and r.Type=3 and r.Last=1) as Reputation,

                    msc.Int1 AS MyScore

                from Transactions c indexed by Transactions_Type_Last_String2_Height
                left join Transactions msc indexed by Transactions_Type_String1_String2_Height
                    on msc.Type in (301) and msc.Height is not null and msc.String2 = c.String2 and msc.String1 = ?
                where c.Type in (204, 205)
                    and c.Last = 1
                    and c.Height is not null
                    )sql" + commentHashesWhere + R"sql(
            )sql";

            TryTransactionStep(func, [&]()
            {
                auto stmt = SetupSqlStatement(sql);

                int i = 1;
                TryBindStatementText(stmt, i++, addressHash);
                for (const auto& commentHashe: commentHashes)
                    TryBindStatementText(stmt, i++, commentHashe);

                LogPrint(BCLog::SQL, "%s: %s\n", func, sqlite3_expanded_sql(*stmt));

                while (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    UniValue record(UniValue::VOBJ);

                    if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("cmntid", value);
                    if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) record.pushKV("scoreUp", value);
                    if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) record.pushKV("scoreDown", value);
                    if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok) record.pushKV("reputation", value);
                    if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("myScore", value);

                    result.push_back(record);
                }

                FinalizeSqlStatement(*stmt);
            });
        }

        return result;
    }

    UniValue GetPostScores(const string& postTxHash)
    {
        // TODO (brangr):
    }

    UniValue WebRpcRepository::GetAddressScores(const vector<string>& postHashes, const string& address)
    {
        UniValue result(UniValue::VARR);

        string postWhere;
        if (!postHashes.empty())
        {
            postWhere += " and s.String2 in ( ";
            postWhere += join(vector<string>(postHashes.size(), "?"), ",");
            postWhere += " ) ";
        }

        string sql = R"sql(
            select
                s.String2 as posttxid,
                s.String1 as address,
                up.String2 as name,
                up.String3 as avatar,
                ur.Value as reputation,
                s.Int1 as value
            from Transactions s
            join Transactions u on u.Type in (100) and u.Height is not null and u.String1 = s.String1 and u.Last = 1
            join Payload up on up.TxHash = u.Hash
            left join (select ur.* from Ratings ur where ur.Type=0 and ur.Last=1) ur on ur.Id = u.Id
            where s.Type in (300)
                and s.String1 = ?
                and s.Height is not null
                )sql" + postWhere + R"sql()
            order by s.Time desc
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, address);

            int i = 2;
            for (const auto& postHash: postHashes)
                TryBindStatementText(stmt, i++, postHash);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("posttxid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) record.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) record.pushKV("name", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok) record.pushKV("avatar", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 4); ok) record.pushKV("reputation", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 5); ok) record.pushKV("value", value);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    map<string, UniValue> WebRpcRepository::GetSubscribesAddresses(const vector<string>& addresses, const vector<TxType>& types)
    {
        auto result = map<string, UniValue>();
        for (const auto& address: addresses)
            result[address] = UniValue(UniValue::VARR);

        string sql = R"sql(
            select
                String1 as AddressHash,
                String2 as AddressToHash,
                case
                    when Type = 303 then 1
                    else 0
                end as Private
            from Transactions
            where Type in ( )sql" + join(types | transformed(static_cast<string(*)(int)>(to_string)), ",") + R"sql( )
              and Last = 1
              and String1 in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            for (const auto& address: addresses)
                TryBindStatementText(stmt, i++, address);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);
                auto[ok, address] = TryGetColumnString(*stmt, 0);

                if (auto[ok1, value] = TryGetColumnString(*stmt, 1); ok1) record.pushKV("adddress", value);
                if (auto[ok2, value] = TryGetColumnString(*stmt, 2); ok2) record.pushKV("private", value);

                result[address].push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    map<string, UniValue> WebRpcRepository::GetSubscribersAddresses(const vector<string>& addresses, const vector<TxType>& types)
    {
        string sql = R"sql(
            select
                String2 as AddressToHash,
                String1 as AddressHash
            from Transactions indexed by Transactions_Type_Last_String2_Height
            where Type in ( )sql" + join(types | transformed(static_cast<string(*)(int)>(to_string)), ",") + R"sql( )
              and Last = 1
              and Height is not null
              and String2 in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
        )sql";

        auto result = map<string, UniValue>();
        for (const auto& address: addresses)
            result[address] = UniValue(UniValue::VARR);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            for (const auto& address: addresses)
                TryBindStatementText(stmt, i++, address);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto[ok, addressTo] = TryGetColumnString(*stmt, 0);
                auto[ok1, address] = TryGetColumnString(*stmt, 1);

                result[addressTo].push_back(address);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    map<string, UniValue> WebRpcRepository::GetBlockingToAddresses(const vector<string>& addresses)
    {
        string sql = R"sql(
            SELECT String1 as AddressHash,
                String2 as AddressToHash
            FROM Transactions
            WHERE Type in (305) and Last = 1
            and String1 in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
        )sql";

        auto result = map<string, UniValue>();
        for (const auto& address: addresses)
            result[address] = UniValue(UniValue::VARR);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            for (const auto& address: addresses)
                TryBindStatementText(stmt, i++, address);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto[ok, address] = TryGetColumnString(*stmt, 0);
                auto[ok1, addressTo] = TryGetColumnString(*stmt, 1);

                result[address].push_back(addressTo);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetTags(const string& lang, int pageSize, int pageStart)
    {
        UniValue result(UniValue::VARR);

        string sql = R"sql(
            select q.Lang, q.Value, q.cnt
            from (
                select t.Lang, t.Value, (select count(1) from web.TagsMap tm where tm.TagId = t.Id)cnt
                from web.Tags t indexed by Tags_Lang_Id
                where t.Lang = ?
            )q
            order by cnt desc
            limit ?
            offset ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            TryBindStatementText(stmt, i++, lang);
            TryBindStatementInt(stmt, i++, pageSize);
            TryBindStatementInt(stmt, i++, pageStart);
                

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto[ok0, vLang] = TryGetColumnString(*stmt, 0);
                auto[ok1, vValue] = TryGetColumnString(*stmt, 1);
                auto[ok2, vCount] = TryGetColumnInt(*stmt, 2);

                UniValue record(UniValue::VOBJ);
                record.pushKV("tag", vValue);
                record.pushKV("count", vCount);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    vector<int64_t> WebRpcRepository::GetContentIds(const vector<string>& txHashes)
    {
        vector<int64_t> result;

        if (txHashes.empty())
            return result;

        string sql = R"sql(
            select Id
            from Transactions
            where Hash in ( )sql" + join(vector<string>(txHashes.size(), "?"), ",") + R"sql( )
              and Height is not null
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            
            int i = 1;
            for (const string& txHash : txHashes)
                TryBindStatementText(stmt, i++, txHash);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    result.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetUnspents(vector<string>& addresses, int height,
        vector<pair<string, uint32_t>>& mempoolInputs)
    {
        UniValue result(UniValue::VARR);

        string sql = R"sql(
            select
                o.TxHash,
                o.Number,
                o.AddressHash,
                o.Value,
                o.ScriptPubKey,
                t.Type,
                o.TxHeight
            from TxOutputs o indexed by TxOutputs_SpentHeight_AddressHash
            join Transactions t on t.Hash=o.TxHash
            where o.AddressHash in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
              and o.TxHeight is not null
              and o.SpentHeight is null
            order by o.TxHeight asc
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            for (const auto& address: addresses)
                TryBindStatementText(stmt, i++, address);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                auto[ok0, txHash] = TryGetColumnString(*stmt, 0);
                auto[ok1, txOut] = TryGetColumnInt(*stmt, 1);

                string _txHash = txHash;
                int _txOut = txOut;
                // Exclude outputs already used as inputs in mempool
                if (!ok0 || !ok1 || find_if(
                    mempoolInputs.begin(),
                    mempoolInputs.end(),
                    [&](const pair<string, uint32_t>& itm)
                    {
                        return itm.first == _txHash && itm.second == _txOut;
                    })  != mempoolInputs.end())
                    continue;

                record.pushKV("txid", txHash);
                record.pushKV("vout", txOut);

                if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) record.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 3); ok)
                {
                    record.pushKV("amount", ValueFromAmount(value));
                    record.pushKV("amountSat", value);
                }
                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("scriptPubKey", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 5); ok)
                {
                    record.pushKV("coinbase", value == 2 || value == 3);
                    record.pushKV("pockettx", value > 3);
                }
                if (auto[ok, value] = TryGetColumnInt(*stmt, 6); ok)
                {
                    record.pushKV("confirmations", height - value);
                    record.pushKV("height", value);
                }

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    tuple<int, UniValue> WebRpcRepository::GetContentLanguages(int height)
    {
        int resultCount = 0;
        UniValue resultData(UniValue::VOBJ);

        string sql = R"sql(
            select c.Type,
                   p.String1 as lang,
                   count(*) as cnt
            from Transactions c
            join Payload p on p.TxHash = c.Hash
            where c.Type in (200, 201)
              and c.Last = 1
              and c.Height is not null
              and c.Height > ?
            group by c.Type, p.String1
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementInt(stmt, 1, height);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto[okType, typeInt] = TryGetColumnInt(*stmt, 0);
                auto[okLang, lang] = TryGetColumnString(*stmt, 1);
                auto[okCount, count] = TryGetColumnInt(*stmt, 2);
                if (!okType || !okLang || !okCount)
                    continue;

                auto type = TransactionHelper::TxStringType((TxType) typeInt);

                if (resultData.At(type).isNull())
                    resultData.pushKV(type, UniValue(UniValue::VOBJ));

                resultData.At(type).pushKV(lang, count);
                resultCount += count;
            }

            FinalizeSqlStatement(*stmt);
        });

        return {resultCount, resultData};
    }

    tuple<int, UniValue> WebRpcRepository::GetLastAddressContent(const string& address, int height, int count)
    {
        int resultCount = 0;
        UniValue resultData(UniValue::VARR);

        // Get count
        string sqlCount = R"sql(
            select count(*)
            from Transactions
            where Type in (200, 201)
              and Last = 1
              and Height is not null
              and Height > ?
              and String1 = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sqlCount);
            TryBindStatementInt(stmt, 1, height);
            TryBindStatementText(stmt, 2, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    resultCount = value;

            FinalizeSqlStatement(*stmt);
        });

        // Try get last N records
        if (resultCount > 0)
        {
            string sql = R"sql(
                select Hash,
                       Time,
                       Height
                from Transactions
                where Type in (200, 201)
                  and Last = 1
                  and Height is not null
                  and Height > ?
                  and String1 = ?
                order by Height desc
                limit ?
            )sql";

            TryTransactionStep(__func__, [&]()
            {
                auto stmt = SetupSqlStatement(sql);
                TryBindStatementInt(stmt, 1, height);
                TryBindStatementText(stmt, 2, address);
                TryBindStatementInt(stmt, 3, count);

                while (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    auto[okHash, hash] = TryGetColumnString(*stmt, 0);
                    auto[okTime, time] = TryGetColumnInt64(*stmt, 1);
                    auto[okHeight, block] = TryGetColumnInt(*stmt, 2);
                    if (!okHash || !okTime || !okHeight)
                        continue;

                    UniValue record(UniValue::VOBJ);
                    record.pushKV("txid", hash);
                    record.pushKV("time", time);
                    record.pushKV("nblock", block);
                    resultData.push_back(record);
                }

                FinalizeSqlStatement(*stmt);
            });
        }

        return {resultCount, resultData};
    }
    
    UniValue WebRpcRepository::GetContentsForAddress(const string& address)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (address.empty())
            return result;

        string sql = R"sql(
            select

                t.Id,
                t.String2 as RootTxHash,
                t.Time,
                p.String2 as Caption,
                p.String3 as Message,
                p.String6 as Settings,
                ifnull(r.Value,0) as Reputation,

                (select count(*) from Transactions scr indexed by Transactions_Type_Last_String2_Height
                    where scr.Type = 300 and scr.Last in (0,1) and scr.Height is not null and scr.String2 = t.String2) as ScoresCount,

                ifnull((select sum(scr.Int1) from Transactions scr indexed by Transactions_Type_Last_String2_Height
                    where scr.Type = 300 and scr.Last in (0,1) and scr.Height is not null and scr.String2 = t.String2),0) as ScoresSum

            from Transactions t indexed by Transactions_Type_Last_String1_Height_Id
            left join Payload p on t.Hash = p.TxHash
            left join Ratings r indexed by Ratings_Type_Id_Last_Height
                on r.Type = 2 and r.Last = 1 and r.Id = t.Id

            where t.Type in (200, 201)
                and t.Last = 1
                and t.String1 = ?
            order by t.Height desc
            limit 50
        )sql";

        TryTransactionStep(func, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementText(stmt, 1, address);

            LogPrint(BCLog::SQL, "%s: %s\n", func, sqlite3_expanded_sql(*stmt));

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                auto[ok0, id] = TryGetColumnInt64(*stmt, 0);
                auto[ok1, hash] = TryGetColumnString(*stmt, 1);
                auto[ok2, time] = TryGetColumnString(*stmt, 2);
                auto[ok3, caption] = TryGetColumnString(*stmt, 3);
                auto[ok4, message] = TryGetColumnString(*stmt, 4);
                auto[ok5, settings] = TryGetColumnString(*stmt, 5);
                auto[ok6, reputation] = TryGetColumnString(*stmt, 6);
                auto[ok7, scoreCnt] = TryGetColumnString(*stmt, 7);
                auto[ok8, scoreSum] = TryGetColumnString(*stmt, 8);
                
                if (ok3) record.pushKV("content", HtmlUtils::UrlDecode(caption));
                else record.pushKV("content", HtmlUtils::UrlDecode(message).substr(0, 100));

                record.pushKV("txid", hash);
                record.pushKV("time", time);
                record.pushKV("reputation", reputation);
                record.pushKV("settings", settings);
                record.pushKV("scoreSum", scoreSum);
                record.pushKV("scoreCnt", scoreCnt);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedRelayedContent(const string& address, int height)
    {
        vector<UniValue> result;

        string sql = R"sql(
            select
                r.Hash,
                r.String3 as RelayTxHash,
                r.String1 as AddressHash,
                r.Time,
                r.Height
            from Transactions r
            join Transactions p on p.Hash = r.String3 and p.String1 = ?
            where r.Type in (200, 201)
              and r.Last = 1
              and r.Height is not null
              and r.Height > ?
              and r.String3 is not null
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementInt(stmt, 1, height);
            TryBindStatementText(stmt, 2, address);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                record.pushKV("msg", "reshare");
                if (auto[ok, val] = TryGetColumnString(*stmt, 0); ok) record.pushKV("txid", val);
                if (auto[ok, val] = TryGetColumnString(*stmt, 1); ok) record.pushKV("txidRepost", val);
                if (auto[ok, val] = TryGetColumnString(*stmt, 2); ok) record.pushKV("addrFrom", val);
                if (auto[ok, val] = TryGetColumnInt64(*stmt, 3); ok) record.pushKV("time", val);
                if (auto[ok, val] = TryGetColumnInt(*stmt, 4); ok) record.pushKV("nblock", val);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedContentsScores(const string& address, int height, int limit)
    {
        vector<UniValue> result;

        string sql = R"sql(
            select
                s.String1 as address,
                s.Hash,
                s.Time,
                s.String2 as posttxid,
                s.Int1 as value,
                s.Height
            from Transactions c indexed by Transactions_Type_Last_String1_Height_Id
            join Transactions s indexed by Transactions_Type_Last_String2_Height
                on s.Type in (300) and s.Last in (0,1) and s.String2 = c.Hash and s.Height is not null and s.Height > ?
            where c.Type in (200, 201)
              and c.Last = 1
              and c.Height is not null
              and c.String1 = ?
            order by s.Time desc
            limit ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementInt(stmt, 1, height);
            TryBindStatementText(stmt, 2, address);
            TryBindStatementInt(stmt, 3, limit);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                record.pushKV("addr", address);
                record.pushKV("msg", "event");
                record.pushKV("mesType", "upvoteShare");
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("addrFrom", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) record.pushKV("txid", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 2); ok) record.pushKV("time", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok) record.pushKV("posttxid", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 4); ok) record.pushKV("upvoteVal", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 5); ok) record.pushKV("nblock", value);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedCommentsScores(const string& address, int height, int limit)
    {
        vector<UniValue> result;

        string sql = R"sql(
            select
                s.String1 as address,
                s.Hash,
                s.Time,
                s.String2 as commenttxid,
                s.Int1 as value,
                s.Height
            from Transactions c indexed by Transactions_Type_Last_String1_Height_Id
            join Transactions s indexed by Transactions_Type_Last_String2_Height
                on s.Type in (301) and s.String2 = c.Hash and s.Height is not null and s.Height > ?
            where c.Type in (204, 205)
              and c.Last = 1
              and c.Height is not null
              and c.String1 = ?
            order by s.Time desc
            limit ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, address);
            TryBindStatementInt(stmt, 2, height);
            TryBindStatementInt(stmt, 3, limit);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                record.pushKV("addr", address);
                record.pushKV("msg", "event");
                record.pushKV("mesType", "cScore");
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("addrFrom", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) record.pushKV("txid", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 2); ok) record.pushKV("time", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok) record.pushKV("commentid", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 4); ok) record.pushKV("upvoteVal", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 5); ok) record.pushKV("nblock", value);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    map<string, UniValue> WebRpcRepository::GetMissedTransactions(const string& address, int height, int count)
    {
        map<string, UniValue> result;

        string sql = R"sql(
            select
                o.TxHash,
                t.Time,
                o.Value,
                o.TxHeight,
                t.Type
            from TxOutputs o indexed by TxOutputs_TxHeight_AddressHash
            join Transactions t on t.Hash = o.TxHash
            where o.AddressHash = ?
              and o.TxHeight > ?
            order by o.TxHeight desc
            limit ?
         )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, address);
            TryBindStatementInt(stmt, 2, height);
            TryBindStatementInt(stmt, 3, count);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto[okTxHash, txHash] = TryGetColumnString(*stmt, 0);
                if (!okTxHash) continue;

                UniValue record(UniValue::VOBJ);
                record.pushKV("txid", txHash);
                record.pushKV("addr", address);
                record.pushKV("msg", "transaction");
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 1); ok) record.pushKV("time", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 2); ok) record.pushKV("amount", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 3); ok) record.pushKV("nblock", value);

                if (auto[ok, value] = TryGetColumnInt(*stmt, 4); ok)
                {
                    auto stringType = TransactionHelper::TxStringType((TxType) value);
                    if (!stringType.empty())
                        record.pushKV("type", stringType);
                }

                result.emplace(txHash, record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedCommentAnswers(const string& address, int height, int count)
    {
        vector<UniValue> result;

        string sql = R"sql(
            select
                c.Hash,
                c.Time,
                c.Height,
                c.String1 as addrFrom,
                c.String3 as posttxid,
                c.String4 as  parentid,
                c.String5 as  answerid
            from Transactions c indexed by Transactions_Type_Last_String1_String2_Height
            join Transactions a indexed by Transactions_Type_Last_Height_String5_String1
                on a.Type in (204, 205) and a.Height > ? and a.Last = 1 and a.String5 = c.String2 and a.String1 != c.String1
            where c.Type in (204, 205)
              and c.Last = 1
              and c.Height is not null
              and c.String1 = ?
            order by a.Height desc
            limit ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, address);
            TryBindStatementInt(stmt, 2, height);
            TryBindStatementInt(stmt, 3, count);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                record.pushKV("addr", address);
                record.pushKV("msg", "comment");
                record.pushKV("mesType", "answer");
                record.pushKV("reason", "answer");
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("txid", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 1); ok) record.pushKV("time", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 2); ok) record.pushKV("nblock", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok) record.pushKV("addrFrom", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("posttxid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) record.pushKV("parentid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 6); ok) record.pushKV("answerid", value);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedPostComments(const string& address, const vector<string>& excludePosts,
        int height, int count)
    {
        vector<UniValue> result;

        string sql = R"sql(
            select
                c.Hash,
                c.Time,
                c.Height,
                c.String1 as addrFrom,
                c.String3 as posttxid,
                c.String4 as  parentid,
                c.String5 as  answerid
            from Transactions p indexed by Transactions_Type_Last_String1_String2_Height
            join Transactions c indexed by Transactions_Type_Last_String3_Height
                on c.Type in (204, 205) and c.Height > ? and c.Last = 1 and c.String3 = p.String2 and c.String1 != p.String1
            where p.Type in (200, 201)
              and p.Last = 1
              and p.Height is not null
              and p.String1 = ?
              and p.Hash not in ( )sql" + join(vector<string>(excludePosts.size(), "?"), ",") + R"sql( )
            order by c.Height desc
            limit ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            TryBindStatementInt(stmt, i++, height);
            TryBindStatementText(stmt, i++, address);
            for (const auto& excludePost: excludePosts)
                TryBindStatementText(stmt, i++, excludePost);
            TryBindStatementInt(stmt, i, count);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                record.pushKV("addr", address);
                record.pushKV("msg", "comment");
                record.pushKV("mesType", "post");
                record.pushKV("reason", "post");
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("txid", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 1); ok) record.pushKV("time", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 2); ok) record.pushKV("nblock", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok) record.pushKV("addrFrom", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("posttxid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) record.pushKV("parentid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 6); ok) record.pushKV("answerid", value);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::SearchLinks(const vector<string>& links, const vector<int>& contentTypes,
        const int nHeight, const int countOut)
    {
        UniValue result(UniValue::VARR);

        if (links.empty())
            return result;

        string contentTypesWhere = join(vector<string>(contentTypes.size(), "?"), ",");
        string  linksWhere = join(vector<string>(links.size(), "?"), ",");

        string sql = R"sql(
            select t.Id
            from Transactions t indexed by Transactions_Type_Last_String1_Height_Id
            join Payload p indexed by Payload_String7 on p.TxHash = t.Hash
            where t.Type in ( )sql" + contentTypesWhere + R"sql( )
                and t.Height <= ?
                and t.Last = 1
                and p.String7 in ( )sql" + linksWhere + R"sql( )
            limit ?
        )sql";

        vector<int64_t> ids;
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);

            TryBindStatementInt(stmt, i++, nHeight);

            for (const auto& link: links)
                TryBindStatementText(stmt, i++, link);

            TryBindStatementInt(stmt, i++, countOut);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    ids.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        if (ids.empty())
            return result;

        auto contents = GetContentsData(ids, "");
        result.push_backV(contents);

        return result;
    }

    UniValue WebRpcRepository::GetContentsStatistic(const vector<string>& addresses, const vector<int>& contentTypes,
        const int nHeight, const int depth)
    {
        UniValue result(UniValue::VARR);

        if (addresses.empty())
            return result;

        string sql = R"sql(
            select

                sum(q.scrSum) as scoreSum,
                sum(q.scrCnt) as scoreCnt,
                count(1) as countLikers

            from (
              select sum(s.Int1)scrSum, count(1)scrCnt

              from Transactions v indexed by Transactions_Type_Last_String1_Height_Id

              join Transactions s indexed by Transactions_Type_Last_String2_Height
                on s.Type in ( 300 ) and s.Last in (0,1) and s.Height > 0 and s.String2 = v.String2

              where v.Type in ( )sql" + join(vector<string>(contentTypes.size(), "?"), ",") + R"sql( )
                and v.Last = 1
                and v.Height <= ?
                and v.Height > ?
                and v.String1 = ?

              group by s.String1
            ) q
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            int i = 1;
            auto stmt = SetupSqlStatement(sql);

            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);
            TryBindStatementInt(stmt, i++, nHeight);
            TryBindStatementInt(stmt, i++, nHeight - depth);
            TryBindStatementText(stmt, i++, addresses[0]);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                record.pushKV("address", addresses[0]);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok) record.pushKV("scoreSum", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 1); ok) record.pushKV("scoreCnt", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 2); ok) record.pushKV("countLikers", value);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    // ------------------------------------------------------
    // Feeds

    UniValue WebRpcRepository::GetHotPosts(int countOut, const int depth, const int nHeight, const string& lang,
        const vector<int>& contentTypes, const string& address, int badReputationLimit)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        string sql = R"sql(
            select t.Id

            from Transactions t indexed by Transactions_Type_Last_String3_Height

            join Payload p indexed by Payload_String1_TxHash
                on p.String1 = ? and t.Hash = p.TxHash

            join Ratings r indexed by Ratings_Type_Id_Last_Value
                on r.Type = 2 and r.Last = 1 and r.Id = t.Id and r.Value > 0

            join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                on u.Type in (100) and u.Last = 1 and u.Height > 0 and u.String1 = t.String1

            left join Ratings ur indexed by Ratings_Type_Id_Last_Height
                on ur.Type = 0 and ur.Last = 1 and ur.Id = u.Id

            where t.Type in ( )sql" + join(vector<string>(contentTypes.size(), "?"), ",") + R"sql( )
                and t.Last = 1
                and t.Height is not null
                and t.Height <= ?
                and t.Height > ?
                and t.String3 is null

                -- Do not show posts from users with low reputation
                and ifnull(ur.Value,0) > ?

            order by r.Value desc
            limit ?
        )sql";

        vector<int64_t> ids;
        TryTransactionStep(func, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            TryBindStatementText(stmt, i++, lang);
            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);
            TryBindStatementInt(stmt, i++, nHeight);
            TryBindStatementInt(stmt, i++, nHeight - depth);
            TryBindStatementInt(stmt, i++, badReputationLimit);
            TryBindStatementInt(stmt, i++, countOut);

            LogPrint(BCLog::SQL, "%s: %s\n", func, sqlite3_expanded_sql(*stmt));

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    ids.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        if (ids.empty())
            return result;

        auto contents = GetContentsData(ids, address);
        result.push_backV(contents);

        return result;
    }

    vector<UniValue> WebRpcRepository::GetContentsData(const vector<int64_t>& ids, const string& address)
    {
        auto func = __func__;
        vector<UniValue> result{};

        if (ids.empty())
            return result;

        string sql = R"sql(
            select
                t.String2 as RootTxHash,
                t.Id,
                case when t.Hash != t.String2 then 'true' else null end edit,
                t.String3 as RelayTxHash,
                t.String1 as AddressHash,
                t.Time,
                p.String1 as Lang,
                t.Type,
                p.String2 as Caption,
                p.String3 as Message,
                p.String7 as Url,
                p.String4 as Tags,
                p.String5 as Images,
                p.String6 as Settings,

                (select count(*) from Transactions scr indexed by Transactions_Type_Last_String2_Height
                    where scr.Type = 300 and scr.Last in (0,1) and scr.Height is not null and scr.String2 = t.String2) as ScoresCount,

                ifnull((select sum(scr.Int1) from Transactions scr indexed by Transactions_Type_Last_String2_Height
                    where scr.Type = 300 and scr.Last in (0,1) and scr.Height is not null and scr.String2 = t.String2),0) as ScoresSum,

                (select count(*) from Transactions rep indexed by Transactions_Type_Last_String3_Height
                    where rep.Type in (200,201) and rep.Last = 1 and rep.Height is not null and rep.String3 = t.String2) as Reposted,

                (select count(*) from Transactions com indexed by Transactions_Type_Last_String3_Height
                    where com.Type in (204,205) and com.Last = 1 and com.Height is not null and com.String3 = t.String2) as CommentsCount,
                
                ifnull((select scr.Int1 from Transactions scr indexed by Transactions_Type_Last_String1_String2_Height
                    where scr.Type = 300 and scr.Last in (0,1) and scr.Height is not null and scr.String1 = ? and scr.String2 = t.String2),0) as MyScore

            from Transactions t indexed by Transactions_Last_Id_Height
            left join Payload p on t.Hash = p.TxHash
            where t.Type in (200, 201, 207)
              and t.Height is not null
              and t.Last = 1
              and t.Id in ( )sql" + join(vector<string>(ids.size(), "?"), ",") + R"sql( )
        )sql";

        // Get posts
        unordered_map<int64_t, UniValue> tmpResult{};
        vector<string> authors;
        TryTransactionStep(func, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            int i = 1;

            TryBindStatementText(stmt, i++, address);

            for (int64_t id : ids)
                TryBindStatementInt64(stmt, i++, id);

            LogPrint(BCLog::SQL, "%s: %s\n", func, sqlite3_expanded_sql(*stmt));

            // ---------------------------
            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                auto[okHash, txHash] = TryGetColumnString(*stmt, 0);
                auto[okId, txId] = TryGetColumnInt64(*stmt, 1);
                record.pushKV("txid", txHash);
                record.pushKV("id", txId);

                if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) record.pushKV("edit", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok) record.pushKV("repost", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok)
                {
                    authors.emplace_back(value);
                    record.pushKV("address", value);
                }
                if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) record.pushKV("time", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 6); ok) record.pushKV("l", value); // lang
                if (auto[ok, value] = TryGetColumnString(*stmt, 8); ok) record.pushKV("c", value); // caption
                if (auto[ok, value] = TryGetColumnString(*stmt, 9); ok) record.pushKV("m", value); // message
                if (auto[ok, value] = TryGetColumnString(*stmt, 10); ok) record.pushKV("u", value); // url
                
                if (auto[ok, value] = TryGetColumnInt(*stmt, 7); ok)
                {
                    record.pushKV("type", TransactionHelper::TxStringType((TxType) value));
                    if ((TxType)value == CONTENT_DELETE)
                        record.pushKV("deleted", "true");
                }

                if (auto[ok, value] = TryGetColumnString(*stmt, 11); ok)
                {
                    UniValue t(UniValue::VARR);
                    t.read(value);
                    record.pushKV("t", t);
                }

                if (auto[ok, value] = TryGetColumnString(*stmt, 12); ok)
                {
                    UniValue ii(UniValue::VARR);
                    ii.read(value);
                    record.pushKV("i", ii);
                }

                if (auto[ok, value] = TryGetColumnString(*stmt, 13); ok)
                {
                    UniValue s(UniValue::VOBJ);
                    s.read(value);
                    record.pushKV("s", s);
                }

                if (auto [ok, value] = TryGetColumnString(*stmt, 14); ok) record.pushKV("scoreCnt", value);
                if (auto [ok, value] = TryGetColumnString(*stmt, 15); ok) record.pushKV("scoreSum", value);
                if (auto [ok, value] = TryGetColumnInt(*stmt, 16); ok && value > 0) record.pushKV("reposted", value);
                if (auto [ok, value] = TryGetColumnInt(*stmt, 17); ok) record.pushKV("comments", value);

                if (!address.empty())
                {
                    if (auto [ok, value] = TryGetColumnString(*stmt, 18); ok)
                        record.pushKV("myVal", value);
                }

                tmpResult[txId] = record;                
            }

            FinalizeSqlStatement(*stmt);
        });

        // ---------------------------------------------
        // Get last comments for all posts
        auto lastComments = GetLastComments(ids, address);
        for (auto& record : tmpResult)
            record.second.pushKV("lastComment", lastComments[record.first]);

        // ---------------------------------------------
        // Get profiles for posts
        auto profiles = GetAccountProfiles(authors, true);
        for (auto& record : tmpResult)
            record.second.pushKV("userprofile", profiles[record.second["address"].get_str()]);

        // ---------------------------------------------
        // Place in result data with source sorting
        for (auto& id : ids)
            result.push_back(tmpResult[id]);

        return result;
    }

    UniValue WebRpcRepository::GetProfileFeed(const string& addressFrom, const string& addressTo, int64_t topContentId,
        int count, const string& lang, const vector<string>& tags, const vector<int>& contentTypes)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (addressTo.empty())
            return result;

        // ---------------------------------------------

        string contentTypesFilter = join(vector<string>(contentTypes.size(), "?"), ",");

        string topContentIdFilter;
        if (topContentId > 0)
            topContentIdFilter = " and t.Id < ? ";

        string sql = R"sql(
            select t.Id
            from Transactions t indexed by Transactions_Type_Last_String1_Height_Id
            where t.Type in ( )sql" + contentTypesFilter + R"sql( )
                and t.Height > 0
                and t.Last = 1
                and t.String1 = ?
                )sql" + topContentIdFilter + R"sql(
        )sql";

        if (!tags.empty())
        {
            sql += R"sql(
                and t.id in (
                    select tm.ContentId
                    from web.Tags tag indexed by Tags_Lang_Value_Id
                    join web.TagsMap tm indexed by TagsMap_TagId_ContentId
                        on tag.Id = tm.TagId
                    where tag.Value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( )
                )
            )sql";
        }

        sql += " order by t.Id desc ";
        sql += " limit ? ";

        // ---------------------------------------------

        vector<int64_t> ids;
        TryTransactionStep(func, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            
            int i = 1;
            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);
            
            TryBindStatementText(stmt, i++, addressTo);

            if (topContentId > 0)
                TryBindStatementInt64(stmt, i++, topContentId);

            if (!tags.empty())
                for (const auto& tag: tags)
                    TryBindStatementText(stmt, i++, tag);

            TryBindStatementInt(stmt, i++, count);

            LogPrint(BCLog::SQL, "%s: %s\n", func, sqlite3_expanded_sql(*stmt));
            
            // ---------------------------------------------

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    ids.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        if (ids.empty())
            return result;

        auto contents = GetContentsData(ids, addressFrom);
        result.push_backV(contents);

        return result;
    }

    UniValue WebRpcRepository::GetSubscribesFeed(const string& addressFrom, int64_t topContentId, int count,
        const string& lang, const vector<string>& tags, const vector<int>& contentTypes)
    {
        // TODO (brangr): add filter by min reputation

        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (addressFrom.empty())
            return result;

        // ---------------------------------------------------

        string contentTypesFilter = join(vector<string>(contentTypes.size(), "?"), ",");

        string topContentIdFilter;
        if (topContentId > 0)
            topContentIdFilter = " and cnt.Id < ? ";

        string sql = R"sql(
            select cnt.Id

            from Transactions cnt indexed by Transactions_Type_Last_String1_Height_Id

            join Transactions subs indexed by Transactions_Type_Last_String1_String2_Height
                on subs.Type in (302,303)
               and subs.Last = 1
               and subs.Height > 0
               and subs.String1 = ?
               and subs.String2 = cnt.String1

            where cnt.Type in ( )sql" + contentTypesFilter + R"sql( )
              and cnt.Last = 1
              and cnt.Height > 0
            
            )sql" + topContentIdFilter + R"sql(

        )sql";

        if (!tags.empty())
        {
            sql += R"sql(
                and cnt.id in (
                    select tm.ContentId
                    from web.Tags tag indexed by Tags_Lang_Value_Id
                    join web.TagsMap tm indexed by TagsMap_TagId_ContentId
                        on tag.Id = tm.TagId
                    where tag.Value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( )
                )
            )sql";
        }
        
        sql += " order by cnt.Id desc ";
        sql += " limit ? ";

        // ---------------------------------------------------

        vector<int64_t> ids;
        TryTransactionStep(func, [&]()
        {
            int i = 1;
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, i++, addressFrom);

            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);
                            
            if (topContentId > 0)
                TryBindStatementInt(stmt, i++, topContentId);
                
            if (!tags.empty())
                for (const auto& tag: tags)
                    TryBindStatementText(stmt, i++, tag);

            TryBindStatementInt(stmt, i++, count);

            LogPrint(BCLog::SQL, "%s: %s\n", func, sqlite3_expanded_sql(*stmt));

            // ---------------------------------------------

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    ids.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        if (ids.empty())
            return result;

        auto contents = GetContentsData(ids, addressFrom);
        result.push_backV(contents);

        return result;
    }

    UniValue WebRpcRepository::GetHistoricalFeed(int countOut, const int64_t& topContentId, int topHeight,
        const string& lang, const vector<string>& tags, const vector<int>& contentTypes,
        const vector<string>& txidsExcluded, const vector<string>& adrsExcluded, const vector<string>& tagsExcluded,
        const string& address, int badReputationLimit)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (contentTypes.empty())
            return result;

        // --------------------------------------------

        string contentTypesWhere = " ( " + join(vector<string>(contentTypes.size(), "?"), ",") + " ) ";

        string contentIdWhere;
        if (topContentId > 0)
            contentIdWhere = " and t.Id < ? ";

        string langFilter;
        if (!lang.empty())
            langFilter += " join Payload p indexed by Payload_String1_TxHash on p.TxHash = t.Hash and p.String1 = ? ";

        string sql = R"sql(
            select t.Id

            from Transactions t indexed by Transactions_Last_Id_Height
            )sql" + langFilter + R"sql(

            join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                on u.Type in (100) and u.Last = 1 and u.Height > 0 and u.String1 = t.String1
            left join Ratings ur indexed by Ratings_Type_Id_Last_Height
                on ur.Type = 0 and ur.Last = 1 and ur.Id = u.Id

            where t.Type in )sql" + contentTypesWhere + R"sql(
                and t.Last = 1
                and t.String3 is null
                and t.Height > 0
                and t.Height <= ?

                -- Do not show posts from users with low reputation
                and ifnull(ur.Value,0) > ?

                )sql" + contentIdWhere + R"sql(
        )sql";

        if (!tags.empty())
        {
            sql += R"sql(
                and t.id in (
                    select tm.ContentId
                    from web.Tags tag indexed by Tags_Lang_Value_Id
                    join web.TagsMap tm indexed by TagsMap_TagId_ContentId
                        on tag.Id = tm.TagId
                    where tag.Value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( )
                        )sql" + (!lang.empty() ? " and tag.Lang = ? " : "") + R"sql(
                )
            )sql";
        }

        if (!txidsExcluded.empty()) sql += " and t.String2 not in ( " + join(vector<string>(txidsExcluded.size(), "?"), ",") + " ) ";
        if (!adrsExcluded.empty()) sql += " and t.String1 not in ( " + join(vector<string>(adrsExcluded.size(), "?"), ",") + " ) ";
        //if (!tagsExcluded.empty()) sql += " and t.Id not in (select tm.ContentId from web.Tags tag join web.TagsMap tm on tag.Id=tm.TagId where tag.Value in ( " + join(vector<string>(tagsExcluded.size(), "?"), ",") + " )) ";

        sql += " order by t.Id desc ";
        sql += " limit ? ";

        // ---------------------------------------------

        vector<int64_t> ids;
        TryTransactionStep(func, [&]()
        {
            int i = 1;
            auto stmt = SetupSqlStatement(sql);

            if (!lang.empty()) TryBindStatementText(stmt, i++, lang);

            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);

            TryBindStatementInt(stmt, i++, topHeight);

            TryBindStatementInt(stmt, i++, badReputationLimit);

            if (topContentId > 0)
                TryBindStatementInt64(stmt, i++, topContentId);

            if (!tags.empty())
            {
                for (const auto& tag: tags)
                    TryBindStatementText(stmt, i++, tag);

                if (!lang.empty())
                    TryBindStatementText(stmt, i++, lang);
            }

            if (!txidsExcluded.empty())
                for (const auto& extxid: txidsExcluded)
                    TryBindStatementText(stmt, i++, extxid);
            
            if (!adrsExcluded.empty())
                for (const auto& exadr: adrsExcluded)
                    TryBindStatementText(stmt, i++, exadr);
            
            // if (!tagsExcluded.empty())
            //     for (const auto& extag: tagsExcluded)
            //         TryBindStatementText(stmt, i++, extag);
                    
            TryBindStatementInt(stmt, i++, countOut);

            LogPrint(BCLog::SQL, "%s: %s\n", func, sqlite3_expanded_sql(*stmt));

            // ---------------------------------------------
            
            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto[ok0, contentId] = TryGetColumnInt64(*stmt, 0);
                ids.push_back(contentId);
            }

            FinalizeSqlStatement(*stmt);
        });

        // Get content data
        if (!ids.empty())
        {
            auto contents = GetContentsData(ids, address);
            result.push_backV(contents);
        }

        // Complete!
        return result;
    }

    UniValue WebRpcRepository::GetHierarchicalFeed(int countOut, const int64_t& topContentId, int topHeight,
        const string& lang, const vector<string>& tags, const vector<int>& contentTypes,
        const vector<string>& txidsExcluded, const vector<string>& adrsExcluded, const vector<string>& tagsExcluded,
        const string& address, int badReputationLimit)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        // ---------------------------------------------

        string contentTypesFilter = join(vector<string>(contentTypes.size(), "?"), ",");

        string langFilter;
        if (!lang.empty())
            langFilter += " join Payload p indexed by Payload_String1_TxHash on p.TxHash = t.Hash and p.String1 = ? ";

        string sql = R"sql(
            select
                (t.Id)ContentId,
                ifnull(pr.Value,0)ContentRating,
                ifnull(ur.Value,0)AccountRating,
                torig.Height,
                
                ifnull((
                    select sum(ifnull(pr.Value,0))
                    from (
                        select p.Id
                        from Transactions p indexed by Transactions_Type_Last_String1_Height_Id
                        where p.Type in ( )sql" + contentTypesFilter + R"sql( )
                            and p.Last = 1
                            and p.String1 = t.String1
                            and p.Height < torig.Height
                            and p.Height > (torig.Height - ?)
                        order by p.Height desc
                        limit ?
                    )q
                    left join Ratings pr indexed by Ratings_Type_Id_Last_Height
                        on pr.Type = 2 and pr.Id = q.Id and pr.Last = 1
                ), 0)SumRating

            from Transactions t indexed by Transactions_Type_Last_String3_Height

            )sql" + langFilter + R"sql(

            join Transactions torig indexed by Transactions_Id on torig.Height > 0 and torig.Id = t.Id and torig.Hash = torig.String2

            left join Ratings pr indexed by Ratings_Type_Id_Last_Height
                on pr.Type = 2 and pr.Last = 1 and pr.Id = t.Id

            join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                on u.Type in (100) and u.Last = 1 and u.Height > 0 and u.String1 = t.String1

            left join Ratings ur indexed by Ratings_Type_Id_Last_Height
                on ur.Type = 0 and ur.Last = 1 and ur.Id = u.Id

            where t.Type in ( )sql" + contentTypesFilter + R"sql( )
                and t.Last = 1
                and t.String3 is null
                and t.Height <= ?
                and t.Height > ?

                -- Do not show posts from users with low reputation
                and ifnull(ur.Value,0) > ?
        )sql";

        if (!tags.empty())
        {
            sql += R"sql( and t.id in (
                select tm.ContentId
                from web.Tags tag indexed by Tags_Lang_Value_Id
                join web.TagsMap tm indexed by TagsMap_TagId_ContentId
                    on tag.Id = tm.TagId
                where tag.Value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( )
                    )sql" + (!lang.empty() ? " and tag.Lang = ? " : "") + R"sql(
            ) )sql";
        }

        if (!txidsExcluded.empty()) sql += " and t.String2 not in ( " + join(vector<string>(txidsExcluded.size(), "?"), ",") + " ) ";
        if (!adrsExcluded.empty()) sql += " and t.String1 not in ( " + join(vector<string>(adrsExcluded.size(), "?"), ",") + " ) ";
        // if (!tagsExcluded.empty()) sql += " and t.Id not in (select tm.ContentId from web.Tags tag join web.TagsMap tm on tag.Id=tm.TagId where tag.Value in ( " + join(vector<string>(tagsExcluded.size(), "?"), ",") + " )) ";

        // ---------------------------------------------
        vector<HierarchicalRecord> postsRanks;
        double dekay = (contentTypes.size() == 1 && contentTypes[0] == CONTENT_VIDEO) ? dekayVideo : dekayContent;

        TryTransactionStep(func, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            int i = 1;

            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);

            TryBindStatementInt(stmt, i++, durationBlocksForPrevPosts);

            TryBindStatementInt(stmt, i++, cntPrevPosts);
            
            if (!lang.empty()) TryBindStatementText(stmt, i++, lang);

            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);

            TryBindStatementInt(stmt, i++, topHeight);
            TryBindStatementInt(stmt, i++, topHeight - cntBlocksForResult);

            TryBindStatementInt(stmt, i++, badReputationLimit);
            
            if (!tags.empty())
            {
                for (const auto& tag: tags)
                    TryBindStatementText(stmt, i++, tag);

                if (!lang.empty())
                    TryBindStatementText(stmt, i++, lang);
            }

            if (!txidsExcluded.empty())
                for (const auto& extxid: txidsExcluded)
                    TryBindStatementText(stmt, i++, extxid);
            
            if (!adrsExcluded.empty())
                for (const auto& exadr: adrsExcluded)
                    TryBindStatementText(stmt, i++, exadr);
            
            if (!tagsExcluded.empty())
                for (const auto& extag: tagsExcluded)
                    TryBindStatementText(stmt, i++, extag);

            LogPrint(BCLog::SQL, "%s: %s\n", func, sqlite3_expanded_sql(*stmt));
            
            // ---------------------------------------------
            
            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                HierarchicalRecord record{};

                auto[ok0, contentId] = TryGetColumnInt64(*stmt, 0);
                auto[ok1, contentRating] = TryGetColumnInt(*stmt, 1);
                auto[ok2, accountRating] = TryGetColumnInt(*stmt, 2);
                auto[ok3, contentOrigHeight] = TryGetColumnInt(*stmt, 3);
                auto[ok4, contentScores] = TryGetColumnInt(*stmt, 4);

                record.Id = contentId;
                record.LAST5 = 1.0 * contentScores;
                record.UREP = accountRating;
                record.PREP = contentRating;
                record.DREP = pow(dekayRep, (topHeight - contentOrigHeight));
                record.DPOST = pow(dekay, (topHeight - contentOrigHeight));

                postsRanks.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        // ---------------------------------------------
        // Calculate content ratings
        int nElements = postsRanks.size();
        for (auto& iPostRank : postsRanks)
        {
            double _LAST5R = 0;
            double _UREPR = 0;
            double _PREPR = 0;

            double boost = 0;
            if (nElements > 1)
            {
                for (auto jPostRank : postsRanks)
                {
                    if (iPostRank.LAST5 > jPostRank.LAST5)
                        _LAST5R += 1;
                    if (iPostRank.UREP > jPostRank.UREP)
                        _UREPR += 1;
                    if (iPostRank.PREP > jPostRank.PREP)
                        _PREPR += 1;
                }

                iPostRank.LAST5R = 1.0 * (_LAST5R * 100) / (nElements - 1);
                iPostRank.UREPR = min(iPostRank.UREP, 1.0 * (_UREPR * 100) / (nElements - 1)) * (iPostRank.UREP < 0 ? 2.0 : 1.0);
                iPostRank.PREPR = min(iPostRank.PREP, 1.0 * (_PREPR * 100) / (nElements - 1)) * (iPostRank.PREP < 0 ? 2.0 : 1.0);
            }
            else
            {
                iPostRank.LAST5R = 100;
                iPostRank.UREPR = 100;
                iPostRank.PREPR = 100;
            }

            iPostRank.POSTRF = 0.4 * (0.75 * (iPostRank.LAST5R + boost) + 0.25 * iPostRank.UREPR) * iPostRank.DREP + 0.6 * iPostRank.PREPR * iPostRank.DPOST;
        }

        // Sort results
        sort(postsRanks.begin(), postsRanks.end(), greater<HierarchicalRecord>());

        // Build result list
        bool found = false;
        int64_t minPostRank = topContentId;
        vector<int64_t> resultIds;
        for(auto iter = postsRanks.begin(); iter < postsRanks.end() && (int)resultIds.size() < countOut; iter++)
        {
            if (iter->Id < minPostRank || minPostRank == 0)
                minPostRank = iter->Id;

            // Find start position
            if (!found && topContentId > 0)
            {
                if (iter->Id == topContentId)
                    found = true;

                continue;
            }

            // Save id for get data
            resultIds.push_back(iter->Id);
        }

        // Get content data
        if (!resultIds.empty())
        {
            auto contents = GetContentsData(resultIds, address);
            result.push_backV(contents);
        }

        // ---------------------------------------------
        // If not completed - request historical data
        int lack = countOut - (int)resultIds.size();
        if (lack > 0)
        {
            UniValue histContents = GetHistoricalFeed(lack, minPostRank, topHeight, lang, tags, contentTypes,
                txidsExcluded, adrsExcluded, tagsExcluded, address, badReputationLimit);

            result.push_backV(histContents.getValues());
        }

        // Complete!
        return result;
    }

}
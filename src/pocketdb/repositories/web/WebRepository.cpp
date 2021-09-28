// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "WebRepository.h"

namespace PocketDb
{
    void WebRepository::Init() {}

    void WebRepository::Destroy() {}

    UniValue WebRepository::GetAddressId(const string& address)
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

    UniValue WebRepository::GetAddressId(int64_t id)
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

    UniValue WebRepository::GetUserAddress(const string& name)
    {
        UniValue result(UniValue::VARR);

        string sql = R"sql(
            SELECT p.String2, u.String1
            FROM Transactions u
            JOIN Payload p on u.Hash = p.TxHash
            WHERE   u.Type in (100, 101, 102)
                and p.String2 = ?
            LIMIT 1
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, name);

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

    UniValue WebRepository::GetAddressesRegistrationDates(const vector<string>& addresses)
    {
        string sql = R"sql(
            WITH addresses (String1, Height) AS (
                SELECT u.String1, MIN(u.Height) AS Height
                FROM Transactions u
                WHERE u.Type in (100, 101, 102)
                and u.Height is not null
                and u.String1 in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
                and u.Last in (0,1)
                GROUP BY u.String1   
            )
            SELECT u.String1, u.Time, u.Hash
            FROM Transactions u
            JOIN addresses a ON u.Type in (100, 101, 102) and u.String1 = a.String1 AND u.Height = a.Height
        )sql";

        auto result = UniValue(UniValue::VARR);

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

    UniValue WebRepository::GetTopAddresses(int count)
    {
        UniValue result(UniValue::VARR);

        auto sql = R"sql(
            select o.AddressHash, sum(o.Value)
            from TxOutputs o indexed by TxOutputs_SpentHeight
            where o.SpentHeight is null
            group by o.AddressHash
            order by sum(o.Value) desc
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

    UniValue WebRepository::GetAccountState(const string& address, int heightWindow)
    {
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(
            select
                u.String1 as Address,
                up.String2 as Name,

                (select reg.Time from Transactions reg indexed by Transactions_Id
                    where reg.Id=u.Id and reg.Height=(select min(reg1.Height) from Transactions reg1 indexed by Transactions_Id where reg1.Id=reg.Id)) as RegistrationDate,

                ifnull((select r.Value from Ratings r where r.Type=0 and r.Id=u.Id and r.Height=(select max(r1.height) from Ratings r1 where r1.Type=0 and r1.Id=r.Id)),0) as Reputation,

                ifnull((select sum(o.Value) from TxOutputs o indexed by TxOutputs_AddressHash_SpentHeight_TxHeight
                    where o.AddressHash=u.String1 and o.SpentHeight is null),0) as Balance,

                (select count(1) from Ratings r where r.Type=1 and r.Id=u.Id) as Likers,

                (select count(1) from Transactions p indexed by Transactions_Type_Last_String1_Height
                    where p.Type in (200) and p.Hash=p.String2 and p.String1=u.String1 and p.Height>=?) as PostSpent,

                (select count(1) from Transactions p indexed by Transactions_Type_Last_String1_Height
                    where p.Type in (201) and p.Hash=p.String2 and p.String1=u.String1 and p.Height>=?) as VideoSpent,

                (select count(1) from Transactions p indexed by Transactions_Type_Last_String1_Height
                    where p.Type in (204) and p.String1=u.String1 and p.Height>=?) as CommentSpent,

                (select count(1) from Transactions p indexed by Transactions_Type_Last_String1_Height
                    where p.Type in (300) and p.String1=u.String1 and p.Height>=?) as ScoreSpent,

                (select count(1) from Transactions p indexed by Transactions_Type_Last_String1_Height
                    where p.Type in (301) and p.String1=u.String1 and p.Height>=?) as ScoreCommentSpent,

                (select count(1) from Transactions p indexed by Transactions_Type_Last_String1_Height
                    where p.Type in (307) and p.String1=u.String1 and p.Height>=?) as ComplainSpent

            from Transactions u indexed by Transactions_Type_Last_String1_Height
            join Payload up on up.TxHash=u.Hash

            where u.Type in (100, 102, 102)
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

    map<string, UniValue> WebRepository::GetUserProfile(const vector<string>& addresses, bool shortForm, int option)
    {
        map<string, UniValue> result{};

        string sql = R"sql(
            select
                u.String1 as Address,
                p.String2 as Name,
                u.Id as AccountId,
                p.String3 as Avatar,
                p.String7 as Donations,
                p.String4 as About,
                ifnull(u.String2,'') as Referrer,
                p.String1 as Lang,
                p.String5 as Url,
                u.Time as LastUpdate,
                p.String6 as Pubkey,

                (select count(1) from Transactions ru indexed by Transactions_Type_Last_String2_Height
                    where ru.Type in (100,101,102) and ru.Last=1 and ru.Height is not null and ru.String2=u.String1) as ReferrersCount,

                (select count(1) from Transactions po indexed by Transactions_Type_Last_String1_Height
                    where po.Type in (200) and po.Last=1 and po.Height is not null and po.String1=u.String1) as PostsCount,

                (select r.Value from Ratings r indexed by Ratings_Height
                    where r.Type=0 and r.Id=u.Id order by r.Height desc limit 1) / 10 as Reputation,

                (select reg.Time from Transactions reg indexed by Transactions_Id
                    where reg.Id=u.Id and reg.Height is not null order by reg.Height asc limit 1) as RegistrationDate

            from Transactions u indexed by Transactions_Type_Last_String1_Height
            join Payload p on p.TxHash=u.Hash
            where u.Type in (100,101,102) and u.Last=1 and u.Height is not null
            and u.String1 in ()sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql()
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            for (int i = 0; i < (int) addresses.size(); i++)
                TryBindStatementText(stmt, i + 1, addresses[i]);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto[ok, address] = TryGetColumnString(*stmt, 0);

                UniValue record(UniValue::VOBJ);

                record.pushKV("address", address);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("name", valueStr);
                if (auto[ok, valueNum] = TryGetColumnInt(*stmt, 2); ok)
                    record.pushKV("id", valueNum); //TODO (brangr): check pls in pocketrpc.cpp was id + 1
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 3); ok) record.pushKV("i", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 4); ok) record.pushKV("b", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 6); ok) record.pushKV("r", valueStr);
                if (auto[ok, valueNum] = TryGetColumnInt(*stmt, 11); ok) record.pushKV("rc", valueNum);
                if (auto[ok, valueNum] = TryGetColumnInt(*stmt, 12); ok) record.pushKV("postcnt", valueNum);
                if (auto[ok, valueNum] = TryGetColumnInt(*stmt, 13); ok) record.pushKV("reputation", valueNum / 10.0);

                if (option == 1)
                {
                    if (auto[ok, valueStr] = TryGetColumnString(*stmt, 5); ok) record.pushKV("a", valueStr);
                }

                if (shortForm)
                {
                    result.insert_or_assign(address, record);
                    continue;
                }

                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 7); ok) record.pushKV("l", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 8); ok) record.pushKV("s", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 9); ok) record.pushKV("update", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 10); ok) record.pushKV("k", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 14); ok) record.pushKV("regdate", valueStr);

                result.insert_or_assign(address, record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRepository::GetLastComments(int count, int height, const string& lang)
    {
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
                (select r.Value from Ratings r where r.Id=t.Id and r.Type=3 and r.Height=(select max(r1.height) from Ratings r1 where r1.Type=3 and r1.Id=r.Id)) as Reputation,
                pl.String2 AS Msg
            from Transactions t
            join RowIds rid on t.RowId = rid.RowId
            join Payload pl ON pl.TxHash = t.Hash
            where t.Type in (204,205) and t.Last=1 and t.height is not null
            order by t.Height desc, t.Time desc
            limit ?;
        )sql";

        auto result = UniValue(UniValue::VARR);

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

    UniValue WebRepository::GetCommentsByPost(const string& postHash, const string& parentHash, const string& addressHash)
    {
        auto result = UniValue(UniValue::VARR);

        auto sql = R"sql(
            SELECT
                c.Type,
                c.Hash,
                c.String2 as RootTxHash,
                c.String3 as PostTxHash,
                c.String1 as AddressHash,
                r.Time AS RootTime,
                c.Time,
                c.Height,
                pl.String2 AS Msg,
                c.String4 as ParentTxHash,
                c.String5 as AnswerTxHash,
                (SELECT COUNT(1) FROM Transactions sc WHERE sc.Type=301 and sc.Height is not null and sc.String2 = c.Hash AND sc.Int1 = 1) as ScoreUp,
                (SELECT COUNT(1) FROM Transactions sc WHERE sc.Type=301 and sc.Height is not null and sc.String2 = c.Hash and sc.Int1 = -1) as ScoreDown,
                (SELECT r.Value FROM Ratings r WHERE r.Id=c.Id AND r.Type=3 and r.Height=(select max(r1.Height) from Ratings r1 where r1.Type=3 and r1.Id=r.Id)) as Reputation,
                IFNULL(sc.Int1, 0) AS MyScore,
                (SELECT COUNT(1) FROM Transactions s WHERE s.Type in (204, 205) and s.Height is not null and s.String4 = c.String2 and s.Last = 1) AS ChildrenCount,
                (SELECT sum(o1.Value) Amount FROM Transactions c1 JOIN Transactions p1 ON p1.Hash = c1.String3 JOIN TxOutputs o1 ON o1.TxHash = c1.Hash and o1.AddressHash = p1.String1 WHERE c1.String2 = c.String2) as Amount
            FROM Transactions c
            JOIN Transactions r ON c.String2 = r.Hash
            JOIN Payload pl ON pl.TxHash = c.Hash
            LEFT JOIN Transactions sc ON sc.Type in (301) and sc.Height is not null and sc.String2 = c.String2 and sc.String1 = ?
            WHERE c.Type in (204, 205)
                and c.Height is not null
                and c.Last = 1
                and c.String3 = ?
                AND (? = '' or c.String4 = ?)
                AND c.Time < ?
        )sql";

        //TODO add donate amount

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, addressHash);
            TryBindStatementText(stmt, 2, postHash);
            TryBindStatementText(stmt, 3, parentHash);
            TryBindStatementText(stmt, 4, parentHash);
            TryBindStatementInt64(stmt, 5, GetAdjustedTime());

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto record = ParseCommentRow(*stmt);
                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRepository::GetCommentsByIds(const string& addressHash, const vector<string>& commentHashes)
    {
        string sql = R"sql(
            SELECT
                c.Type,
                c.Hash,
                c.String2 as RootTxHash,
                c.String3 as PostTxHash,
                c.String1 as AddressHash,
                r.Time AS RootTime,
                c.Time,
                c.Height,
                pl.String2 AS Msg,
                c.String4 as ParentTxHash,
                c.String5 as AnswerTxHash,
                (SELECT COUNT(1) FROM Transactions sc WHERE sc.Type=301 and sc.Height is not null and sc.String2 = c.Hash AND sc.Int1 = 1) as ScoreUp,
                (SELECT COUNT(1) FROM Transactions sc WHERE sc.Type=301 and sc.Height is not null and sc.String2 = c.Hash and sc.Int1 = -1) as ScoreDown,
                (SELECT r.Value FROM Ratings r WHERE r.Id=c.Id AND r.Type=3 and r.Height=(select max(r1.Height) from Ratings r1 where r1.Type=3 and r1.Id=r.Id)) as Reputation,
                IFNULL(sc.Int1, 0) AS MyScore,
                (SELECT COUNT(1) FROM Transactions s WHERE s.Type in (204, 205) and s.Height is not null and s.String4 = c.String2 and s.Last = 1) AS ChildrenCount,
                (SELECT sum(o1.Value) Amount FROM Transactions c1 JOIN Transactions p1 ON p1.Hash = c1.String3 JOIN TxOutputs o1 ON o1.TxHash = c1.Hash and o1.AddressHash = p1.String1 WHERE c1.String2 = c.String2) as Amount
            FROM Transactions c
            JOIN Transactions r on r.Type in (204,205) and r.Height is not null and r.Hash=c.String2
            JOIN Payload pl ON pl.TxHash = c.Hash
            LEFT JOIN Transactions sc on sc.Type in (301) and sc.Height is not null and sc.String2=c.String2 and sc.String1=?
            WHERE c.Type in (204,205)
                and c.Height is not null
                and c.Last = 1
                and c.Time < ?
                and c.String2 in ()sql" + join(vector<string>(commentHashes.size(), "?"), ",") + R"sql()
        )sql";

        //TODO add donate amount

        auto result = UniValue(UniValue::VARR);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, addressHash);
            TryBindStatementInt64(stmt, 2, GetAdjustedTime());

            for (size_t i = 0; i < commentHashes.size(); i++)
                TryBindStatementText(stmt, (int) i + 3, commentHashes[i]);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto record = ParseCommentRow(*stmt);
                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRepository::GetPostScores(const vector<string>& postHashes, const string& addressHash)
    {
        auto result = UniValue(UniValue::VARR);

        string sql = R"sql(
            select
                sc.String2, -- post
                sc.String1, -- address
                p.String2, -- name
                p.String3, -- avatar
                (select r.Value from Ratings r where r.Type=0 and r.Id=u.Id order by Height desc limit 1), -- user reputation
                sc.Int1, -- score value
                case
                    when sub.Type is null then null
                    when sub.Type = 303 then 1
                    else 0
                end -- isPrivate
            from Transactions sc indexed by Transactions_Type_Last_String1_String2_Height

            join Transactions u indexed by Transactions_Type_Last_String1_String2_Height
                on u.Type in (100, 101, 102) and u.Last = 1 and u.String1=sc.String1 and u.Height is not null

            join Payload p ON p.TxHash=u.Hash

            left join Transactions sub indexed by Transactions_Type_Last_String1_String2_Height
                on sub.Type in (302, 303) and sub.Last=1 and sub.Height is not null and sub.String2=sc.String1 AND sub.String1=?

            where sc.Type in (300) and sc.Height is not null
                and sc.String2 in ()sql" + join(vector<string>(postHashes.size(), "?"), ",") + R"sql()
            order by sub.Type is null, sc.Time
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, addressHash);

            int i = 2;
            for (const auto& postHash : postHashes)
                TryBindStatementText(stmt, i++, postHash);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("posttxid", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("address", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("name", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 3); ok) record.pushKV("avatar", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 4); ok) record.pushKV("reputation", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 5); ok) record.pushKV("value", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 6); ok) record.pushKV("isprivate", valueStr);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRepository::GetPageScores(const vector<string>& commentHashes, const string& addressHash)
    {
        string sql = R"sql(
            select
            c.String2 as RootTxHash,
            (select count(1) from Transactions sc WHERE sc.Type in (301) and sc.Height is not null and sc.String2 = c.Hash AND sc.Int1 = 1) as ScoreUp,
            (select count(1) from Transactions sc WHERE sc.Type in (301) and sc.Height is not null and sc.String2 = c.Hash AND sc.Int1 = -1) as ScoreDown,
            (select r.Value from Ratings r where r.Id=c.Id and r.Type=3 and r.Height=(select max(r1.Height) from Ratings r1 where r1.Id=r.Id and r1.Type=3)) as Reputation,
            msc.Int1 AS MyScore
            from Transactions c
            left join Transactions msc on msc.Type in (301) and msc.Height is not null and msc.String2 = c.String2 and msc.String1=?
            where c.Type in (204, 205)
                and c.Last = 1
                and c.Time < ?
        )sql";

        sql += "AND c.RootTxHash IN ('";
        sql += commentHashes[0];
        sql += "'";
        for (size_t i = 1; i < commentHashes.size(); i++)
        {
            sql += ",'";
            sql += commentHashes[i];
            sql += "'";
        }
        sql += ")";

        auto result = UniValue(UniValue::VARR);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, addressHash);
            TryBindStatementInt64(stmt, 2, GetAdjustedTime());

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("cmntid", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("scoreUp", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("scoreDown", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 3); ok) record.pushKV("reputation", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 4); ok) record.pushKV("myscore", valueStr);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRepository::GetAddressScores(const vector<string>& postHashes, const string& address)
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
            join Transactions u
                on u.Type in (100) and u.Height is not null and u.String1 = s.String1 and u.Last = 1
            join Payload up on up.TxHash = u.Hash
            left join (select ur.* from Ratings ur where ur.Type = 0
                    and ur.Height = (select max(ur1.Height) from Ratings ur1 where ur1.Type = ur.Type and ur1.Id = ur.Id)) ur
                on ur.Id = u.Id
            where s.Type in (300)
                and s.String1 = ?
                )sql" + postWhere + R"sql()
            order by s.Time desc
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, address);

            int i = 2;
            for (const auto& postHash : postHashes)
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

    UniValue WebRepository::ParseCommentRow(sqlite3_stmt* stmt)
    {
        UniValue record(UniValue::VOBJ);

        //auto[ok0, txHash] = TryGetColumnString(stmt, 1);
        auto[ok1, rootTxHash] = TryGetColumnString(stmt, 2);
        record.pushKV("id", rootTxHash);

        if (auto[ok, valueStr] = TryGetColumnString(stmt, 3); ok)
            record.pushKV("postid", valueStr);

        auto[ok8, msgValue] = TryGetColumnString(stmt, 8);
        record.pushKV("msg", msgValue);

        if (auto[ok, valueStr] = TryGetColumnString(stmt, 4); ok) record.pushKV("address", valueStr);
        if (auto[ok, valueStr] = TryGetColumnString(stmt, 5); ok) record.pushKV("time", valueStr);
        if (auto[ok, valueStr] = TryGetColumnString(stmt, 6); ok) record.pushKV("timeUpd", valueStr);
        if (auto[ok, valueStr] = TryGetColumnString(stmt, 7); ok) record.pushKV("block", valueStr);
        if (auto[ok, valueStr] = TryGetColumnString(stmt, 9); ok) record.pushKV("parentid", valueStr);
        if (auto[ok, valueStr] = TryGetColumnString(stmt, 10); ok) record.pushKV("answerid", valueStr);
        if (auto[ok, valueStr] = TryGetColumnString(stmt, 11); ok) record.pushKV("scoreUp", valueStr);
        if (auto[ok, valueStr] = TryGetColumnString(stmt, 12); ok) record.pushKV("scoreDown", valueStr);
        if (auto[ok, valueStr] = TryGetColumnString(stmt, 13); ok) record.pushKV("reputation", valueStr);
        if (auto[ok, valueStr] = TryGetColumnString(stmt, 14); ok) record.pushKV("myScore", valueStr);
        if (auto[ok, valueStr] = TryGetColumnString(stmt, 15); ok) record.pushKV("children", valueStr);
        if (auto[ok, valueStr] = TryGetColumnString(stmt, 16); ok)
        {
            if (valueStr != "0")
            {
                record.pushKV("amount", valueStr);
                record.pushKV("donation", "true");
            }
        }

        if (auto[ok, value] = TryGetColumnInt(stmt, 0); ok)
        {
            switch (static_cast<PocketTxType>(value))
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

        return record;
    }

    map<string, UniValue> WebRepository::GetSubscribesAddresses(const vector<string>& addresses, const vector<PocketTxType>& types)
    {
        auto result = map<string, UniValue>();
        for (const auto& address : addresses)
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
            where Type in ()sql" + join(types | transformed(static_cast<std::string(*)(int)>(std::to_string)), ",") + R"sql()
              and Last = 1
              and String1 in ()sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql()
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            for (const auto& address : addresses)
                TryBindStatementText(stmt, i++, address);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);
                auto[ok, address] = TryGetColumnString(*stmt, 0);

                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) record.pushKV("adddress", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) record.pushKV("private", value);

                result[address].push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    map<string, UniValue> WebRepository::GetSubscribersAddresses(const vector<string>& addresses, const vector<PocketTxType>& types)
    {
        string sql = R"sql(
            select
                String2 as AddressToHash,
                String1 as AddressHash
            from Transactions
            where Type in ()sql" + join(types | transformed(static_cast<std::string(*)(int)>(std::to_string)), ",") + R"sql()
              and Last = 1
              and String2 in ()sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql()
        )sql";

        auto result = map<string, UniValue>();
        for (const auto& address : addresses)
            result[address] = UniValue(UniValue::VARR);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            for (const auto& address : addresses)
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

    map<string, UniValue> WebRepository::GetBlockingToAddresses(const vector<string>& addresses)
    {
        string sql = R"sql(
            SELECT String1 as AddressHash,
                String2 as AddressToHash
            FROM Transactions
            WHERE Type in (305) and Last = 1
            and String1 in ()sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql()
        )sql";

        auto result = map<string, UniValue>();
        for (const auto& address : addresses)
            result[address] = UniValue(UniValue::VARR);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            for (const auto& address : addresses)
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

    map<string, UniValue> WebRepository::GetContents(int nHeight, const string& start_txid, int countOut, const string& lang,
        const vector<string>& tags, const vector<int>& contentTypes, const vector<string>& txidsExcluded,
        const vector<string>& adrsExcluded, const vector<string>& tagsExcluded, const string& address)
    {
        string sql = R"sql(
            SELECT
                t.String2 as RootTxHash,
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
                p.String6 as Settings
            FROM Transactions t indexed by Transactions_Height_Time
            JOIN Payload p on t.Hash = p.TxHash
            where t.Id > ifnull((select max(t0.Id) from Transactions t0 indexed by Transactions_Type_Last_String2_Height where t0.Type in (200, 201) and t0.String2 = ? and t0.Last = 1),0)
                and t.Last = 1
                and t.Height <= ?
                and t.Time <= ?
                and t.String3 is null
                and p.String1 = ?
                and t.Type in (200, 201)
            order by t.Height desc, t.Time desc
            limit ?
        )sql";

        map<string, UniValue> result{};
        std::vector<std::string> authors;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, start_txid);
            TryBindStatementInt(stmt, 2, nHeight);
            TryBindStatementInt64(stmt, 3, GetAdjustedTime());
            TryBindStatementText(stmt, 4, lang);
            TryBindStatementInt(stmt, 5, countOut);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                auto[ok, txid] = TryGetColumnString(*stmt, 0);
                record.pushKV("txid", txid);

                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) record.pushKV("edit", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) record.pushKV("repost", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok)
                {
                    authors.emplace_back(value);
                    record.pushKV("address", value);
                    //UniValue userprofile(UniValue::VARR);
                    //auto userprofile = GetUserProfile({value});
                    //record.pushKV("userprofile", userprofile[0]);
                }
                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("time", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) record.pushKV("l", value); // lang
                if (auto[ok, value] = TryGetColumnString(*stmt, 6); ok) record.pushKV("type", value == "201" ? "video" : "share");
                if (auto[ok, value] = TryGetColumnString(*stmt, 7); ok) record.pushKV("c", value); // caption
                if (auto[ok, value] = TryGetColumnString(*stmt, 8); ok) record.pushKV("m", value); // message
                if (auto[ok, value] = TryGetColumnString(*stmt, 9); ok) record.pushKV("u", value); // url

                if (auto[ok, value] = TryGetColumnString(*stmt, 10); ok)
                {
                    UniValue t(UniValue::VARR);
                    t.read(value);
                    record.pushKV("t", t);
                }

                if (auto[ok, value] = TryGetColumnString(*stmt, 11); ok)
                {
                    UniValue i(UniValue::VARR);
                    i.read(value);
                    record.pushKV("i", i);
                }

                if (auto[ok, value] = TryGetColumnString(*stmt, 12); ok)
                {
                    UniValue s(UniValue::VOBJ);
                    s.read(value);
                    record.pushKV("s", s);
                }

                record.pushKV("scoreSum", "0");//if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("scoreSum", valueStr);
                record.pushKV("scoreCnt", "0");//if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("scoreCnt", valueStr);
                //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("myVal", valueStr);
                record.pushKV("comments", 0);//if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("comments", valueStr);
                //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("lastComment", valueStr);
                //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("reposted", valueStr);

                result.insert_or_assign(txid, record);
            }

            FinalizeSqlStatement(*stmt);
        });

        auto profiles = GetUserProfile(authors);
        for (const auto& item : result)
        {
            std::string useradr = item.second["address"].get_str();
            result[item.first].pushKV("userprofile", profiles[useradr]);
        }

        return result;
    }

/*
map<string, UniValue> GetContentsData(vector<string>& txids)
{
    string sql = R"sql(
        SELECT c.RootTxHash,
               case when c.Hash!=c.RootTxHash then 'true' else '' end edit,
               c.RelayTxHash,
               c.AddressHash,
               c.Time,
               c.Lang,
               c.Type,
               c.Caption,
               c.Message,
               c.Url,
               c.Tags,
               c.Images,
               c.Settings
        FROM vWebContents c
        WHERE 1 = 1
    )sql";

    sql += "AND c.RootTxHash IN ('";
    sql += txids[0];
    sql += "'";
    for (size_t i = 1; i < txids.size(); i++)
    {
        sql += ",'";
        sql += txids[i];
        sql += "'";
    }
    sql += ")";

    map<string, UniValue> result{};

    TryTransactionStep(__func__, [&]() {
      auto stmt = SetupSqlStatement(sql);

      while (sqlite3_step(*stmt) == SQLITE_ROW)
      {
          UniValue record(UniValue::VOBJ);

          auto [ok, txid] = TryGetColumnString(*stmt, 0);
          record.pushKV("txid", txid);

          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("edit", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("repost", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 3); ok) record.pushKV("address", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 4); ok) record.pushKV("time", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 5); ok) record.pushKV("l", valueStr); // lang
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 6); ok) record.pushKV("type", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 7); ok) record.pushKV("c", valueStr); // caption
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 8); ok) record.pushKV("m", valueStr); // message
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 9); ok) record.pushKV("u", valueStr); // url
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 10); ok) {
              UniValue t(UniValue::VARR);
              record.pushKV("t", t);
          }
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 11); ok) {
              UniValue t(UniValue::VARR);
              record.pushKV("i", t);
          }
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 12); ok) record.pushKV("settings", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("scoreSum", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("scoreCnt", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("myVal", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("comments", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("lastComment", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("reposted", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("userprofile", valueStr);

//          record.pushKV("address", address);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("name", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("id", valueStr); //TODO (brangr): check pls in pocketrpc.cpp was id + 1
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 3); ok) record.pushKV("i", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 4); ok) record.pushKV("b", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 6); ok) record.pushKV("r", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 11); ok) record.pushKV("rc", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 12); ok) record.pushKV("postcnt", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 13); ok) record.pushKV("reputation", valueStr);
//
////          if (option == 1)
////          {
////              if (auto [ok, valueStr] = TryGetColumnString(*stmt, 5); ok) record.pushKV("a", valueStr);
////          }
////
////          if (shortForm)
////          {
////              result.insert_or_assign(address, record);
////              continue;
////          }
//
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 7); ok) record.pushKV("l", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 8); ok) record.pushKV("s", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 9); ok) record.pushKV("update", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 10); ok) record.pushKV("k", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 14); ok) record.pushKV("regdate", valueStr);

          result.insert_or_assign(txid, record);
      }

      FinalizeSqlStatement(*stmt);
    });

    return result;
}

map<string, UniValue> GetContents(map<string, param>& conditions, optional<int> &counttotal)
{
    string sql = R"sql(
        SELECT c.RootTxHash,
               case when c.Hash!=c.RootTxHash then 'true' else '' end edit,
               c.RelayTxHash,
               c.AddressHash,
               c.Time,
               c.Lang,
               c.Type,
               c.Caption,
               c.Message,
               c.Url,
               c.Tags,
               c.Images,
               c.Settings
        FROM vWebContents c
        WHERE 1 = 1
    )sql";

    sql += strprintf(" AND c.Time <= %d", GetAdjustedTime());
    sql += " AND c.RelayTxHash is null";
    if (conditions.count("height")) sql += strprintf(" AND c.Height <= %i", conditions["height"].get_int());
    if (conditions.count("lang")) sql += strprintf(" AND c.Lang = '%s'", conditions["lang"].get_str().c_str());
    //if (conditions.count("tags")) sql += strprintf(" AND c.Tags = '%s'", conditions["tags"].get_str().c_str());
    if (conditions.count("type") && conditions["types"].get_vint().size()) {
        sql += " AND c.Type in (";
        sql += conditions["types"].get_vint()[0];
        for (size_t i = 1; i < conditions["types"].get_vint().size(); i++)
        {
            sql += ",";
            sql += conditions["types"].get_vint()[i];
        }
        sql += ")";
    }
    sql += " order by c.Height desc, c.Time desc";
    sql += strprintf(" limit %i", conditions.count("count") ? conditions["count"].get_int() : 10);

    map<string, UniValue> result{};

    TryTransactionStep(__func__, [&]() {
      auto stmt = SetupSqlStatement(sql);

      while (sqlite3_step(*stmt) == SQLITE_ROW)
      {
          UniValue record(UniValue::VOBJ);

          auto [ok, txid] = TryGetColumnString(*stmt, 0);
          record.pushKV("txid", txid);

          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("edit", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("repost", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 3); ok) record.pushKV("address", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 4); ok) record.pushKV("time", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 5); ok) record.pushKV("l", valueStr); // lang
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 6); ok) record.pushKV("type", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 7); ok) record.pushKV("c", valueStr); // caption
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 8); ok) record.pushKV("m", valueStr); // message
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 9); ok) record.pushKV("u", valueStr); // url
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 10); ok) {
              UniValue t(UniValue::VARR);
              record.pushKV("t", t);
          }
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 11); ok) {
              UniValue t(UniValue::VARR);
              record.pushKV("i", t);
          }
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 12); ok) record.pushKV("settings", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("scoreSum", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("scoreCnt", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("myVal", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("comments", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("lastComment", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("reposted", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("userprofile", valueStr);

//          record.pushKV("address", address);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("name", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("id", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 3); ok) record.pushKV("i", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 4); ok) record.pushKV("b", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 6); ok) record.pushKV("r", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 11); ok) record.pushKV("rc", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 12); ok) record.pushKV("postcnt", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 13); ok) record.pushKV("reputation", valueStr);
//
////          if (option == 1)
////          {
////              if (auto [ok, valueStr] = TryGetColumnString(*stmt, 5); ok) record.pushKV("a", valueStr);
////          }
////
////          if (shortForm)
////          {
////              result.insert_or_assign(address, record);
////              continue;
////          }
//
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 7); ok) record.pushKV("l", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 8); ok) record.pushKV("s", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 9); ok) record.pushKV("update", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 10); ok) record.pushKV("k", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 14); ok) record.pushKV("regdate", valueStr);

          result.insert_or_assign(txid, record);
      }

      FinalizeSqlStatement(*stmt);
    });

    return result;
}

map<string, UniValue> GetContents(map<string, param>& conditions)
{
    //return this->GetContents(conditions, &nullopt);
}*/

    UniValue WebRepository::GetUnspents(vector<string>& addresses, int height)
    {
        UniValue result(UniValue::VARR);

        string sql = R"sql(
            select
                o.TxHash,
                o.Number,
                o.AddressHash,
                o.Value,
                o.ScriptPubKey,
                t.Type
            from TxOutputs o
            join Transactions t on t.Hash=o.TxHash
            where o.AddressHash in ()sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql()
                and o.SpentHeight is null
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            for (const auto& address : addresses)
                TryBindStatementText(stmt, i++, address);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("txid", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 1); ok) record.pushKV("vout", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) record.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 3); ok) record.pushKV("amount", ValueFromAmount(value));
                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("scriptPubKey", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 5); ok) record.pushKV("confirmations", height - value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 6); ok)
                {
                    record.pushKV("coinbase", value == 2 || value == 3);
                    record.pushKV("pockettx", value > 3);
                }

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    tuple<int, UniValue> WebRepository::GetContentLanguages(int height)
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

                auto type = TransactionHelper::TxStringType((PocketTxType) typeInt);

                if (resultData.At(type).isNull())
                {
                    UniValue lang(UniValue::VOBJ);
                    resultData.pushKV(type, lang);
                }

                resultData.At(type).pushKV(lang, count);
                resultCount += count;
            }

            FinalizeSqlStatement(*stmt);
        });

        return {resultCount, resultData};
    }

    tuple<int, UniValue> WebRepository::GetLastAddressContent(const string& address, int height, int count)
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
                    auto[okHeight, height] = TryGetColumnInt(*stmt, 2);
                    if (!okHash || !okTime || !okHeight)
                        continue;

                    UniValue record(UniValue::VOBJ);
                    record.pushKV("txid", hash);
                    record.pushKV("time", time);
                    record.pushKV("nblock", height);
                    resultData.push_back(record);
                }

                FinalizeSqlStatement(*stmt);
            });
        }

        return {resultCount, resultData};
    }

}
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_USER_HPP
#define POCKETCONSENSUS_USER_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/dto/User.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  User consensus base class
    *
    *******************************************************************************************************************/
    class UserConsensus : public SocialBaseConsensus
    {
    public:
        UserConsensus(int height) : SocialBaseConsensus(height) {}
        UserConsensus() : SocialBaseConsensus() {}

        tuple<bool, SocialConsensusResult> Validate(shared_ptr<Transaction> tx, PocketBlock& block) override
        {
            return Validate(static_pointer_cast<User>(tx), block);
        }

        tuple<bool, SocialConsensusResult> Check(shared_ptr<Transaction> tx) override
        {
            if (auto[ok, result] = SocialBaseConsensus::Check(tx); !ok)
                return make_tuple(ok, result);

            return Check(static_pointer_cast<User>(tx));
        }

    protected:

        virtual int64_t GetChangeInfoTimeout() { return 3600; }


        // Check chain consensus rules
        tuple<bool, SocialConsensusResult> Validate(shared_ptr<User> tx, PocketBlock& block)
        {
            if (auto[ok, result] = ValidateDoubleName(tx); !ok)
                return make_tuple(false, result);

            if (auto[ok, result] = ValidateEditProfileLimit(tx); !ok)
                return make_tuple(false, result);
            
            if (auto[ok, result] = ValidateEditProfileBlockLimit(tx, block); !ok)
                return make_tuple(false, result);

            return make_tuple(true, SocialConsensusResult_Success);
        }


        virtual tuple<bool, SocialConsensusResult> ValidateDoubleName(shared_ptr<User> tx)
        {
            if (ConsensusRepoInst.ExistsAnotherByName(*tx->GetAddress(), *tx->GetPayloadName()))
                return make_tuple(false, SocialConsensusResult_NicknameDouble);
        }

        virtual bool ValidateEditProfileLimit(shared_ptr<User> tx, shared_ptr<User> prevTx)
        {
            return (*tx->GetTime() - *prevTx->GetTime()) > GetChangeInfoTimeout();
        }

        virtual tuple<bool, SocialConsensusResult> ValidateEditProfileLimit(shared_ptr<User> tx)
        {
            auto prevTx = ConsensusRepoInst.GetLastAccountTransaction(*tx->GetAddress());
            if (!prevTx)
                return make_tuple(false, SocialConsensusResult_Failed);

            // First user account transaction allowe without next checks
            if (prevTx == nullptr)
                return make_tuple(true, SocialConsensusResult_Success);

            // For edit user profile referrer not allowed
            if (tx->GetReferrerAddress() != nullptr)
                return make_tuple(false, SocialConsensusResult_ReferrerAfterRegistration);

            // We allowe edit profile only with delay
            if (!ValidateEditProfileLimit(tx, static_pointer_cast<User>(prevTx)))
                return make_tuple(false, SocialConsensusResult_ChangeInfoLimit);

            return make_tuple(true, SocialConsensusResult_Success);
        }

        virtual tuple<bool, SocialConsensusResult> ValidateEditProfileBlockLimit(shared_ptr<User> tx, PocketBlock& block)
        {
            // TODO (brangr): Need?
            // по идее мемпул лежит в базе на равне с остальными транзакциями
            // но сюда может насыпаться очень много всего и мы не можем проверить
            // двойное редактирование, тк при загрузке из сети свободных транзакций нет привязки к блоку
            // и поэтому либо разрешаем все такие транзакции либо думаем дальше

            // TODO (brangr): проверка по времени в мемпуле - по идее не нужна

            // if (checkMempool) {
            //     reindexer::QueryResults res;
            //     if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, "Users").Not().Where("txid", CondEq, _txid), res).ok()) {
            //         for (auto& m : res) {
            //             reindexer::Item mItm = m.GetItem();
            //             std::string t_src = DecodeBase64(mItm["data"].As<string>());

            //             reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Users");
            //             if (t_itm.FromJSON(t_src).ok()) {
            //                 if (t_itm["address"].As<string>() == _address) {
            //                     if (!checkWithTime || t_itm["time"].As<int64_t>() <= _time) {
            //                         result = ANTIBOTRESULT::ChangeInfoLimit;
            //                         return false;
            //                     }
            //                 }
            //             }
            //         }
            //     }
            // }

            // TODO (brangr): implement
            // if (blockVtx.Exists("Users")) {
            //     for (auto& mtx : blockVtx.Data["Users"]) {
            //         if (mtx["address"].get_str() == _address && mtx["txid"].get_str() != _txid) {
            //             result = ANTIBOTRESULT::ChangeInfoLimit;
            //             return false;
            //         }
            //     }
            // }

        }


        tuple<bool, SocialConsensusResult> CheckOpReturnHash(shared_ptr<Transaction> tx) override
        {
            // TODO (brangr): implement
            if (auto[baseCheckOk, baseCheckResult] = SocialBaseConsensus::CheckOpReturnHash(tx); !baseCheckOk)
            {
                //     if (table != "Users" || (table == "Users" && vasm[2] != oitm["data_hash_without_ref"].get_str())) {
                //         LogPrintf("FailedOpReturn vasm: %s - oitm: %s\n", vasm[2], oitm.write());
                //         resultCode = ANTIBOTRESULT::FailedOpReturn;
                //         return;
                //     }
            }
        }

    private:
        // Check op_return hash
        tuple<bool, SocialConsensusResult> Check(shared_ptr<User> tx)
        {
            // TODO (brangr): implement for users
            // if (*tx->GetAddress() == *tx->GetReferrerAddress())
            //     return make_tuple(false, SocialConsensusResult_ReferrerSelf);

            // TODO (brangr): implement for users
            // if (_name.size() < 1 && _name.size() > 35) {
            //     result = ANTIBOTRESULT::NicknameLong;
            //     return false;
            // }

            // TODO (brangr): implement for users
            // if (boost::algorithm::ends_with(_name, "%20") || boost::algorithm::starts_with(_name, "%20")) {
            //     result = ANTIBOTRESULT::Failed;
            //     return false;
            // }
        }
        
    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint at 1180000 block
    *
    *******************************************************************************************************************/
    class UserConsensus_checkpoint_1180000 : public UserConsensus
    {
    protected:
        int CheckpointHeight() override { return 1180000; }

        bool ValidateEditProfileLimit(shared_ptr<User> tx, shared_ptr<User> prevTx) override
        {
            return (*tx->GetHeight() - *prevTx->GetHeight()) > GetChangeInfoTimeout();
        }

    public:
        UserConsensus_checkpoint_1180000(int height) : UserConsensus(height) {}
    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint at ?? block
    *
    *******************************************************************************************************************/
    class UserConsensus_checkpoint_ : public UserConsensus
    {
    protected:
        int CheckpointHeight() override { return 0; }

        // Starting from this block, we disable the uniqueness of Name
        virtual tuple<bool, SocialConsensusResult> CheckDoubleName(shared_ptr<User> tx)
        {
            return make_tuple(true, SocialConsensusResult_Success);
        }

    public:
        UserConsensus_checkpoint_(int height) : UserConsensus(height) {}
    };

    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class UserConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<UserConsensus*(int height)>> m_rules =
        {
            {1180000, [](int height) { return new UserConsensus(height); }},
            {0,       [](int height) { return new UserConsensus(height); }},
        };
    public:
        shared_ptr <UserConsensus> Instance(int height)
        {
            return shared_ptr<UserConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }

        shared_ptr <UserConsensus> Instance()
        {
            return shared_ptr<UserConsensus>(new UserConsensus());
        }
    };
}

#endif // POCKETCONSENSUS_USER_HPP

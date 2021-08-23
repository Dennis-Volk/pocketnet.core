// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_USER_HPP
#define POCKETCONSENSUS_USER_HPP

#include "pocketdb/consensus/Social.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/dto/User.hpp"

namespace PocketConsensus
{
    typedef shared_ptr<User> UserRef;

    /*******************************************************************************************************************
    *  User consensus base class
    *******************************************************************************************************************/
    class UserConsensus : public SocialConsensus<UserRef>
    {
    public:
        UserConsensus(int height) : SocialConsensus(height) {}

        ConsensusValidateResult Validate(const UserRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(ptx, block); !baseValidate)
                return {false, baseValidateCode};
                
            // TODO (brangr) (v0.21.0): unique names disabled in future
            if (ConsensusRepoInst.ExistsAnotherByName(*ptx->GetAddress(), *ptx->GetPayloadName()))
            {
                PocketHelpers::SocialCheckpoints socialCheckpoints;
                if (!socialCheckpoints.IsCheckpoint(*ptx->GetHash(), *ptx->GetType(), SocialConsensusResult_NicknameDouble))
                    return {false, SocialConsensusResult_NicknameDouble};
            }

            return ValidateEdit(ptx);
        }

        ConsensusValidateResult Check(const UserRef& ptx) override
        {
            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};

            // Check payload
            if (!ptx->GetPayload()) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetPayloadName())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetPayloadAvatar())) return {false, SocialConsensusResult_Failed};

            // Self referring
            if (!IsEmpty(ptx->GetReferrerAddress()) && *ptx->GetAddress() == *ptx->GetReferrerAddress())
                return make_tuple(false, SocialConsensusResult_ReferrerSelf);

            // Maximum length for user name
            auto name = *ptx->GetPayloadName();
            if (name.empty() || name.size() > 35)
            {
                PocketHelpers::SocialCheckpoints socialCheckpoints;
                if (!socialCheckpoints.IsCheckpoint(*ptx->GetHash(), *ptx->GetType(), SocialConsensusResult_NicknameLong))
                    LogPrintf("$$$ SocialConsensusResult_NicknameLong - %s\n", *ptx->GetHash());
                //return {false, SocialConsensusResult_NicknameLong};
            }

            // Trim spaces
            if (boost::algorithm::ends_with(name, "%20") || boost::algorithm::starts_with(name, "%20"))
            {
                PocketHelpers::SocialCheckpoints socialCheckpoints;
                if (!socialCheckpoints.IsCheckpoint(*ptx->GetHash(), *ptx->GetType(), SocialConsensusResult_Failed))
                    LogPrintf("$$$ SPACES CHECKPOINTS - %s\n", *ptx->GetHash());
                //return {false, SocialConsensusResult_Failed};
            }

            return Success;
        }


    protected:
        virtual int64_t GetChangeInfoDepth() { return 3600; }


        virtual ConsensusValidateResult ValidateEdit(const UserRef& ptx)
        {
            // First user account transaction allowed without next checks
            auto[prevOk, prevTx] = ConsensusRepoInst.GetLastAccount(*ptx->GetAddress());
            if (!prevOk)
                return Success;

            // We allow edit profile only with delay
            if ((*ptx->GetTime() - *prevTx->GetTime()) <= GetChangeInfoDepth())
                return {false, SocialConsensusResult_ChangeInfoLimit};

            // For edit user profile referrer not allowed
            if (ptx->GetReferrerAddress() != nullptr)
                return {false, SocialConsensusResult_ReferrerAfterRegistration};

            return Success;
        }

        ConsensusValidateResult ValidateBlock(const UserRef& ptx, const PocketBlockRef& block) override
        {
            // Only one transaction allowed in block
            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {ACCOUNT_USER}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetAddress() == *blockTx->GetString1())
                {
                    PocketHelpers::SocialCheckpoints socialCheckpoints;
                    if (!socialCheckpoints.IsCheckpoint(*ptx->GetHash(), *ptx->GetType(), SocialConsensusResult_ChangeInfoLimit))
                        return {false, SocialConsensusResult_ChangeInfoLimit};
                }
            }

            return Success;
        }

        ConsensusValidateResult ValidateMempool(const UserRef& ptx) override
        {
            if (ConsensusRepoInst.CountMempoolUser(*ptx->GetAddress()) > 0)
                return {false, SocialConsensusResult_ChangeInfoLimit};

            return Success;
        }

        vector<string> GetAddressesForCheckRegistration(const UserRef& ptx) override
        {
            return {};
        }
    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint at 1180000 block
    *
    *******************************************************************************************************************/
    class UserConsensus_checkpoint_1180000 : public UserConsensus
    {
    public:
        UserConsensus_checkpoint_1180000(int height) : UserConsensus(height) {}
    protected:
        int64_t GetChangeInfoDepth() override { return 60; }
        ConsensusValidateResult ValidateEdit(const UserRef& ptx) override
        {
            // First user account transaction allowed without next checks
            auto[ok, prevTxHeight] = ConsensusRepoInst.GetLastAccountHeight(*ptx->GetAddress());
            if (!ok) return Success;

            // We allow edit profile only with delay
            if ((Height - prevTxHeight) <= GetChangeInfoDepth())
                return {false, SocialConsensusResult_ChangeInfoLimit};

            // For edit user profile referrer not allowed
            if (ptx->GetReferrerAddress() != nullptr)
                return {false, SocialConsensusResult_ReferrerAfterRegistration};

            return Success;
        }
    };

    /*******************************************************************************************************************
    *  Start checkpoint at ?? block
    *******************************************************************************************************************/
    class UserConsensus_checkpoint_ : public UserConsensus_checkpoint_1180000
    {
    public:
        UserConsensus_checkpoint_(int height) : UserConsensus_checkpoint_1180000(height) {}
    protected:
        // TODO (brangr) (v0.21.0): Starting from this block, we disable the uniqueness of Name
        virtual ConsensusValidateResult CheckDoubleName(const UserRef& ptx)
        {
            return make_tuple(true, SocialConsensusResult_Success);
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class UserConsensusFactory : public SocialConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint> _rules = {
            {0,       -1, [](int height) { return make_shared<UserConsensus>(height); }},
            {1180000, 0,  [](int height) { return make_shared<UserConsensus_checkpoint_1180000>(height); }},
        };
    protected:
        const vector<ConsensusCheckpoint>& m_rules() override { return _rules; }
    };

} // namespace PocketConsensus

#endif // POCKETCONSENSUS_USER_HPP

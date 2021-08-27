// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMMENT_HPP
#define POCKETCONSENSUS_COMMENT_HPP

#include "utils/html.h"
#include "pocketdb/ReputationConsensus.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/Comment.h"

namespace PocketConsensus
{
    using namespace std;
    typedef shared_ptr<Comment> CommentRef;

    /*******************************************************************************************************************
    *  Comment consensus base class
    *******************************************************************************************************************/
    class CommentConsensus : public SocialConsensus<Comment>
    {
    public:
        CommentConsensus(int height) : SocialConsensus<Comment>(height) {}
        ConsensusValidateResult Validate(const CommentRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(ptx, block); !baseValidate)
                return {false, baseValidateCode};

            // Parent comment
            if (!IsEmpty(ptx->GetParentTxHash()))
                if (!PocketDb::TransRepoInst.ExistsByHash(*ptx->GetParentTxHash()))
                    return {false, SocialConsensusResult_InvalidParentComment};

            // Answer comment
            if (!IsEmpty(ptx->GetAnswerTxHash()))
                if (!PocketDb::TransRepoInst.ExistsByHash(*ptx->GetAnswerTxHash()))
                    return {false, SocialConsensusResult_InvalidAnswerComment};

            // Check exists content transaction
            auto contentTx = PocketDb::TransRepoInst.GetByHash(*ptx->GetPostTxHash());
            if (!contentTx)
                return {false, SocialConsensusResult_NotFound};
            auto contentPtx = static_pointer_cast<Comment>(contentTx);

            // Check Blocking
            if (auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(
                    *contentPtx->GetAddress(), *ptx->GetAddress()
                ); existsBlocking && blockingType == ACTION_BLOCKING)
                return {false, SocialConsensusResult_Blocking};

            return Success;
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const CommentRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetPostTxHash())) return {false, SocialConsensusResult_Failed};

            // Maximum for message data
            if (!ptx->GetPayload()) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetPayloadMsg())) return {false, SocialConsensusResult_Failed};
            if (HtmlUtils::UrlDecode(*ptx->GetPayloadMsg()).length() > (size_t)GetConsensusLimit(ConsensusLimit_max_comment_size))
                return {false, SocialConsensusResult_Size};

            return Success;
        }

    protected:
        ConsensusValidateResult ValidateBlock(const CommentRef& ptx, const PocketBlockRef& block) override
        {
            int count = GetChainCount(ptx);
            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_COMMENT}))
                    continue;

                auto blockPtx = static_pointer_cast<Comment>(blockTx);

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                {
                    if (CheckBlockLimitTime(ptx, blockPtx))
                        count += 1;
                }
            }

            return ValidateLimit(ptx, count);
        }
        ConsensusValidateResult ValidateMempool(const CommentRef& ptx) override
        {
            int count = GetChainCount(ptx);
            count += ConsensusRepoInst.CountMempoolComment(*ptx->GetAddress());
            return ValidateLimit(ptx, count);
        }
        vector<string> GetAddressesForCheckRegistration(const CommentRef& ptx) override
        {
            return {*ptx->GetAddress()};
        }

        virtual int64_t GetLimit(AccountMode mode) { 
            return mode >= AccountMode_Full ? 
                GetConsensusLimit(ConsensusLimit_full_comment) : 
                GetConsensusLimit(ConsensusLimit_trial_comment); 
        }
        virtual bool CheckBlockLimitTime(const CommentRef& ptx, const CommentRef& blockPtx)
        {
            return *blockPtx->GetTime() <= *ptx->GetTime();
        }
        virtual ConsensusValidateResult ValidateLimit(const CommentRef& ptx, int count)
        {
            auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(Height);
            auto[mode, reputation, balance] = reputationConsensus->GetAccountInfo(*ptx->GetAddress());
            if (count >= GetLimit(mode))
                return {false, SocialConsensusResult_CommentLimit};

            return Success;
        }
        virtual int GetChainCount(const CommentRef& ptx)
        {
            return ConsensusRepoInst.CountChainCommentTime(
                *ptx->GetAddress(),
                *ptx->GetTime() - GetConsensusLimit(ConsensusLimit_depth)
            );
        }
    };

    /*******************************************************************************************************************
    *  Start checkpoint at 1124000 block
    *******************************************************************************************************************/
    class CommentConsensus_checkpoint_1124000 : public CommentConsensus
    {
    public:
        CommentConsensus_checkpoint_1124000(int height) : CommentConsensus(height) {}
    protected:
        bool CheckBlockLimitTime(const CommentRef& ptx, const CommentRef& blockPtx) override
        {
            return true;
        }
    };

    /*******************************************************************************************************************
    *  Start checkpoint at 1180000 block
    *******************************************************************************************************************/
    class CommentConsensus_checkpoint_1180000 : public CommentConsensus_checkpoint_1124000
    {
    public:
        CommentConsensus_checkpoint_1180000(int height) : CommentConsensus_checkpoint_1124000(height) {}
    protected:
        int GetChainCount(const CommentRef& ptx) override
        {
            return ConsensusRepoInst.CountChainCommentHeight(*ptx->GetAddress(), Height - (int) GetConsensusLimit(ConsensusLimit_depth));
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class CommentConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint < CommentConsensus>> m_rules = {
            { 0, -1, [](int height) { return make_shared<CommentConsensus>(height); }},
            { 1124000, -1, [](int height) { return make_shared<CommentConsensus_checkpoint_1124000>(height); }},
            { 1180000, 0, [](int height) { return make_shared<CommentConsensus_checkpoint_1180000>(height); }},
        };
    public:
        shared_ptr<CommentConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<CommentConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(height);
        }
    };
}

#endif // POCKETCONSENSUS_COMMENT_HPP
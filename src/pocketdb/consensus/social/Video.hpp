// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_VIDEO_HPP
#define POCKETCONSENSUS_VIDEO_HPP

#include "pocketdb/consensus/Reputation.hpp"
#include "pocketdb/consensus/Social.hpp"
#include "pocketdb/models/dto/Video.hpp"

namespace PocketConsensus
{
    typedef shared_ptr<Video> VideoRef;

    /*******************************************************************************************************************
    *  Video consensus base class
    *******************************************************************************************************************/
    class VideoConsensus : public SocialConsensus<VideoRef>
    {
    public:
        VideoConsensus(int height) : SocialConsensus(height) {}

        ConsensusValidateResult Validate(const VideoRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(ptx, block); !baseValidate)
                return {false, baseValidateCode};

            if (ptx->IsEdit())
                return ValidateEdit(ptx);

            return Success;
        }

        ConsensusValidateResult Check(const VideoRef& ptx) override
        {
            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};

            // Repost not allowed
            if (!IsEmpty(ptx->GetRelayTxHash())) return {false, SocialConsensusResult_NotAllowed};

            return Success;
        }

    protected:
        virtual int GetLimitWindow() { return 1440; }
        virtual int64_t GetEditWindow() { return 1440; }
        virtual int64_t GetProLimit() { return 100; }
        virtual int64_t GetFullLimit() { return 30; }
        virtual int64_t GetTrialLimit() { return 15; }
        virtual int64_t GetEditLimit() { return 5; }
        virtual int64_t GetLimit(AccountMode mode)
        {
            return mode == AccountMode_Pro
                   ? GetProLimit()
                   : mode == AccountMode_Full
                     ? GetFullLimit()
                     : GetTrialLimit();
        }

        virtual ConsensusValidateResult ValidateEdit(const VideoRef& ptx)
        {
            // First get original post transaction
            auto originalTx = PocketDb::TransRepoInst.GetByHash(*ptx->GetRootTxHash());
            if (!originalTx)
                return {false, SocialConsensusResult_NotFound};

            // Change type not allowed
            if (*originalTx->GetType() != *ptx->GetType())
                return {false, SocialConsensusResult_NotAllowed};

            // You are author? Really?
            if (*ptx->GetAddress() != *originalTx->GetString1())
                return {false, SocialConsensusResult_ContentEditUnauthorized};

            // Original post edit only 24 hours
            if (!AllowEditWindow(ptx, originalTx))
                return {false, SocialConsensusResult_ContentEditLimit};

            return make_tuple(true, SocialConsensusResult_Success);
        }

        // ------------------------------------------------------------------------------------------------------------

        ConsensusValidateResult ValidateBlock(const VideoRef& ptx, const PocketBlockRef& block) override
        {
            // Edit
            if (ptx->IsEdit())
                return ValidateEditBlock(ptx, block);

            // ---------------------------------------------------------
            // New

            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from block
            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_VIDEO}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetAddress() == *blockTx->GetString1())
                    count += 1;
            }

            return ValidateLimit(ptx, count);
        }

        ConsensusValidateResult ValidateMempool(const VideoRef& ptx) override
        {
            // Edit
            if (ptx->IsEdit())
                return ValidateEditMempool(ptx);

            // ---------------------------------------------------------
            // New

            // Get count from chain
            int count = GetChainCount(ptx);

            // and from mempool
            count += ConsensusRepoInst.CountMempoolVideo(*ptx->GetAddress());

            return ValidateLimit(ptx, count);
        }

        virtual ConsensusValidateResult ValidateLimit(const VideoRef& tx, int count)
        {
            auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(Height);

            auto[mode, reputation, balance] = reputationConsensus->GetAccountInfo(*tx->GetAddress());
            auto limit = GetLimit(mode);

            if (count >= limit)
                return {false, SocialConsensusResult_ContentLimit};

            return Success;
        }

        virtual int GetChainCount(const VideoRef& ptx)
        {
            return ConsensusRepoInst.CountChainVideoHeight(
                *ptx->GetAddress(),
                Height - GetLimitWindow()
            );
        }


        virtual ConsensusValidateResult ValidateEditBlock(const VideoRef& ptx, const PocketBlockRef& block)
        {
            // Double edit in block not allowed
            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_VIDEO}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetRootTxHash() == *blockTx->GetString2())
                    return {false, SocialConsensusResult_DoubleContentEdit};
            }

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }

        virtual ConsensusValidateResult ValidateEditMempool(const VideoRef& ptx)
        {
            if (ConsensusRepoInst.CountMempoolVideoEdit(*ptx->GetAddress(), *ptx->GetRootTxHash()) > 0)
                return {false, SocialConsensusResult_DoubleContentEdit};

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }

        virtual ConsensusValidateResult ValidateEditOneLimit(const VideoRef& ptx)
        {
            int count = ConsensusRepoInst.CountChainVideoEdit(*ptx->GetAddress(), *ptx->GetRootTxHash());
            if (count >= GetEditLimit())
                return {false, SocialConsensusResult_ContentEditLimit};

            return Success;
        }

        virtual bool AllowEditWindow(const VideoRef& ptx, const PTransactionRef& originalTx)
        {
            auto[ok, originalTxHeight] = ConsensusRepoInst.GetTransactionHeight(*originalTx->GetHash());
            if (!ok)
                return false;

            return (Height - originalTxHeight) <= GetEditWindow();
        }

        vector<string> GetAddressesForCheckRegistration(const VideoRef& ptx) override
        {
            return {*ptx->GetAddress()};
        }

    };

    /*******************************************************************************************************************
    *  Start checkpoint at 1324655 block
    *******************************************************************************************************************/
    class VideoConsensus_checkpoint_1324655 : public VideoConsensus
    {
    public:
        VideoConsensus_checkpoint_1324655(int height) : VideoConsensus(height) {}
    protected:
        int64_t GetTrialLimit() override { return 5; }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class VideoConsensusFactory : public SocialConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint> _rules = {
            {0,       -1, [](int height) { return make_shared<VideoConsensus>(height); }},
            {1324655, 0,  [](int height) { return make_shared<VideoConsensus_checkpoint_1324655>(height); }},
        };
    protected:
        const vector<ConsensusCheckpoint>& m_rules() override { return _rules; }
    };
}

#endif // POCKETCONSENSUS_VIDEO_HPP
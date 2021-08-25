// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/consensus/Helper.h"

namespace PocketConsensus
{
    bool SocialConsensusHelper::Validate(const PocketBlockRef& block, int height)
    {
        for (const auto& ptx : *block)
        {
            if (auto[ok, result] = validateT(ptx, block, height); !ok)
                return false;
        }

        return true;
    }

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::Validate(const PTransactionRef& tx, int height)
    {
        return validate(tx, nullptr, height);
    }

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::Validate(const PTransactionRef& tx, PocketBlockRef& block, int height)
    {
        return validate(tx, block, height);
    }

    // Проверяет блок транзакций без привязки к цепи
    bool SocialConsensusHelper::Check(const CBlock& block, const PocketBlockRef& pBlock)
    {
        auto coinstakeBlock = find_if(block.vtx.begin(), block.vtx.end(), [&](CTransactionRef const& tx)
        {
            return tx->IsCoinStake();
        }) != block.vtx.end();

        for (const auto& tx : block.vtx)
        {
            // NOT_SUPPORTED transactions not checked
            auto txType = PocketHelpers::ParseType(tx);
            if (txType == PocketTxType::NOT_SUPPORTED)
                continue;
            if (coinstakeBlock && txType == PocketTxType::TX_COINBASE)
                continue;

            // Maybe payload not exists?
            auto txHash = tx->GetHash().GetHex();
            auto it = find_if(pBlock->begin(), pBlock->end(), [&](PTransactionRef const& ptx) { return *ptx == txHash; });
            if (it == pBlock->end())
                return false;

            // Check founded payload
            if (auto[ok, result] = check(tx, *it); !ok)
                return false;
        }

        return true;
    }

    // Проверяет транзакцию без привязки к цепи
    tuple<bool, SocialConsensusResult> SocialConsensusHelper::Check(const CTransactionRef& tx, const PTransactionRef& ptx)
    {
        return check(tx, ptx);
    }

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::validate(const PTransactionRef& ptx, const PocketBlockRef& block, int height)
    {
        auto txType = *ptx->GetType();

        if (!isConsensusable(txType))
            return {true, SocialConsensusResult_Success};

        auto consensus = getConsensus(txType, height);
        if (!consensus)
        {
            LogPrintf("Warning: SocialConsensus type %d not found for transaction %s\n",
                (int) txType, *ptx->GetHash());

            return {false, SocialConsensusResult_Unknown};
        }

        if (auto[ok, result] = consensus->Validate(ptx, block); !ok)
        {
            LogPrintf("Warning: SocialConsensus %d validate failed with result %d for transaction %s with block at height %d\n",
                (int) txType, (int) result, *ptx->GetHash(), height);

            return {false, result};
        }

        return {true, SocialConsensusResult_Success};
    }

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::check(const CTransactionRef& tx, const PTransactionRef& ptx)
    {
        auto txType = *ptx->GetType();

        if (!isConsensusable(txType))
            return {true, SocialConsensusResult_Success};

        auto consensus = getConsensus(txType);
        if (!consensus)
        {
            LogPrintf("Warning: SocialConsensus type %d not found for transaction %s\n",
                (int) txType, *ptx->GetHash());

            return {false, SocialConsensusResult_Unknown};
        }

        if (auto[ok, result] = consensus->Check(tx, ptx); !ok)
        {
            LogPrintf("Warning: SocialConsensus %d check failed with result %d for transaction %s\n",
                (int) txType, (int) result, *ptx->GetHash());

            return {false, result};
        }

        return {true, SocialConsensusResult_Success};
    }

    bool SocialConsensusHelper::isConsensusable(PocketTxType txType)
    {
        switch (txType)
        {
            case TX_COINBASE:
            case TX_COINSTAKE:
            case TX_DEFAULT:
                return false;
            default:
                return true;
        }
    }

    shared_ptr<SocialConsensus> SocialConsensusHelper::getConsensus(PocketTxType txType, int height)
    {
        switch (txType)
        {
            case ACCOUNT_USER:
                return PocketConsensus::UserConsensusFactoryInst.Instance(height);
            case CONTENT_POST:
                return PocketConsensus::PostConsensusFactoryInst.Instance(height);
            case CONTENT_VIDEO:
                return PocketConsensus::VideoConsensusFactoryInst.Instance(height);
            case CONTENT_COMMENT:
                return PocketConsensus::CommentConsensusFactoryInst.Instance(height);
            case CONTENT_COMMENT_EDIT:
                return PocketConsensus::CommentEditConsensusFactoryInst.Instance(height);
            case CONTENT_COMMENT_DELETE:
                return PocketConsensus::CommentDeleteConsensusFactoryInst.Instance(height);
            case ACTION_SCORE_CONTENT:
                return PocketConsensus::ScoreContentConsensusFactoryInst.Instance(height);
            case ACTION_SCORE_COMMENT:
                return PocketConsensus::ScoreCommentConsensusFactoryInst.Instance(height);
            case ACTION_SUBSCRIBE:
                return PocketConsensus::SubscribeConsensusFactoryInst.Instance(height);
            case ACTION_SUBSCRIBE_PRIVATE:
                return PocketConsensus::SubscribePrivateConsensusFactoryInst.Instance(height);
            case ACTION_SUBSCRIBE_CANCEL:
                return PocketConsensus::SubscribeCancelConsensusFactoryInst.Instance(height);
            case ACTION_BLOCKING:
                return PocketConsensus::BlockingConsensusFactoryInst.Instance(height);
            case ACTION_BLOCKING_CANCEL:
                return PocketConsensus::BlockingCancelConsensusFactoryInst.Instance(height);
            case ACTION_COMPLAIN:
                return PocketConsensus::ComplainConsensusFactoryInst.Instance(height);
            case ACCOUNT_VIDEO_SERVER:
            case ACCOUNT_MESSAGE_SERVER:
            case CONTENT_TRANSLATE:
            case CONTENT_SERVERPING:
                // TODO (brangr): future realize types
                break;
            default:
                break;
        }

        return nullptr;
    }

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::validateT(const PTransactionRef& ptx, const PocketBlockRef& block, int height)
    {
        tuple<bool, SocialConsensusResult> result;
        switch (*ptx->GetType())
        {
            // case ACCOUNT_USER:
            //     return PocketConsensus::UserConsensusFactoryInst.Instance(height);
            case CONTENT_POST:
            {
                PostConsensusFactoryT factory;
                auto consensus = factory.Instance(height);
                result = consensus->Validate(static_pointer_cast<Post>(ptx), block);
            }
            // case CONTENT_VIDEO:
            //     return PocketConsensus::VideoConsensusFactoryInst.Instance(height);
            // case CONTENT_COMMENT:
            //     return PocketConsensus::CommentConsensusFactoryInst.Instance(height);
            // case CONTENT_COMMENT_EDIT:
            //     return PocketConsensus::CommentEditConsensusFactoryInst.Instance(height);
            // case CONTENT_COMMENT_DELETE:
            //     return PocketConsensus::CommentDeleteConsensusFactoryInst.Instance(height);
            // case ACTION_SCORE_CONTENT:
            //     return PocketConsensus::ScoreContentConsensusFactoryInst.Instance(height);
            // case ACTION_SCORE_COMMENT:
            //     return PocketConsensus::ScoreCommentConsensusFactoryInst.Instance(height);
            // case ACTION_SUBSCRIBE:
            //     return PocketConsensus::SubscribeConsensusFactoryInst.Instance(height);
            // case ACTION_SUBSCRIBE_PRIVATE:
            //     return PocketConsensus::SubscribePrivateConsensusFactoryInst.Instance(height);
            // case ACTION_SUBSCRIBE_CANCEL:
            //     return PocketConsensus::SubscribeCancelConsensusFactoryInst.Instance(height);
            // case ACTION_BLOCKING:
            //     return PocketConsensus::BlockingConsensusFactoryInst.Instance(height);
            // case ACTION_BLOCKING_CANCEL:
            //     return PocketConsensus::BlockingCancelConsensusFactoryInst.Instance(height);
            // case ACTION_COMPLAIN:
            //     return PocketConsensus::ComplainConsensusFactoryInst.Instance(height);
            case ACCOUNT_VIDEO_SERVER:
            case ACCOUNT_MESSAGE_SERVER:
            case CONTENT_TRANSLATE:
            case CONTENT_SERVERPING:
                // TODO (brangr): future realize types
                break;
            default:
                break;
        }

        if (auto[ok, code] = result; !ok)
        {
            LogPrintf("Warning: SocialConsensus %d validate failed with result %d for transaction %s with block at height %d\n",
                *ptx->GetTypeInt(), (int) code, *ptx->GetHash(), height);

            return {false, code};
        }

        return {true, SocialConsensusResult_Success};
    }
}

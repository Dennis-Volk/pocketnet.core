// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/consensus/Helper.h"

namespace PocketConsensus
{
    PostConsensusFactory SocialConsensusHelper::m_postFactory;
    AccountSettingConsensusFactory SocialConsensusHelper::m_accountSettingFactory;
    UserConsensusFactory SocialConsensusHelper::m_userFactory;
    VideoConsensusFactory SocialConsensusHelper::m_videoFactory;
    CommentConsensusFactory SocialConsensusHelper::m_commentFactory;
    CommentEditConsensusFactory SocialConsensusHelper::m_commentEditFactory;
    CommentDeleteConsensusFactory SocialConsensusHelper::m_commentDeleteFactory;
    ScoreContentConsensusFactory SocialConsensusHelper::m_scoreContentFactory;
    ScoreCommentConsensusFactory SocialConsensusHelper::m_scoreCommentFactory;
    SubscribeConsensusFactory SocialConsensusHelper::m_subscribeFactory;
    SubscribePrivateConsensusFactory SocialConsensusHelper::m_subscribePrivateFactory;
    SubscribeCancelConsensusFactory SocialConsensusHelper::m_subscribeCancelFactory;
    BlockingConsensusFactory SocialConsensusHelper::m_blockingFactory;
    BlockingCancelConsensusFactory SocialConsensusHelper::m_blockingCancelFactory;
    ComplainConsensusFactory SocialConsensusHelper::m_complainFactory;
    ContentDeleteConsensusFactory SocialConsensusHelper::m_contentDeleteFactory;

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::Validate(const PocketBlockRef& block, int height)
    {
        for (const auto& ptx : *block)
        {
            // Not double validate for already in DB
            if (auto[ok, result] = validate(ptx, block, height); !ok)
            {
                LogPrint(BCLog::CONSENSUS, "Warning: SocialConsensus %d validate failed with result %d for block transaction %s at height %d\n",
                    (int) *ptx->GetType(), (int)result, *ptx->GetHash(), height);

                return {false, result};
            }
        }

        return {true, SocialConsensusResult_Success};
    }

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::Validate(const PTransactionRef& ptx, int height)
    {
        // Not double validate for already in DB
        if (TransRepoInst.Exists(*ptx->GetHash()))
            return {true, SocialConsensusResult_Success};

        if (auto[ok, result] = validate(ptx, nullptr, height); !ok)
        {
            LogPrint(BCLog::CONSENSUS, "Warning: SocialConsensus %d validate failed with result %d for transaction %s at height %d\n",
                (int) *ptx->GetType(), (int)result, *ptx->GetHash(), height);

            return {false, result};
        }

        return {true, SocialConsensusResult_Success};
    }

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::Validate(const PTransactionRef& ptx, PocketBlockRef& block, int height)
    {
        if (auto[ok, result] = validate(ptx, block, height); !ok)
        {
            LogPrint(BCLog::CONSENSUS, "Warning: SocialConsensus %d validate failed with result %d for transaction %s at height %d in block %d\n",
                (int) *ptx->GetType(), (int)result, *ptx->GetHash(), height, (block == nullptr ? 1 : 0));

            return {false, result};
        }

        return {true, SocialConsensusResult_Success};
    }

    // Проверяет блок транзакций без привязки к цепи
    tuple<bool, SocialConsensusResult> SocialConsensusHelper::Check(const CBlock& block, const PocketBlockRef& pBlock)
    {
        auto coinstakeBlock = find_if(block.vtx.begin(), block.vtx.end(), [&](CTransactionRef const& tx)
        {
            return tx->IsCoinStake();
        }) != block.vtx.end();

        for (const auto& tx : block.vtx)
        {
            // NOT_SUPPORTED transactions not checked
            auto txType = PocketHelpers::TransactionHelper::ParseType(tx);
            if (txType == TxType::NOT_SUPPORTED)
                continue;
            if (coinstakeBlock && txType == TxType::TX_COINBASE)
                continue;

            // Maybe payload not exists?
            auto txHash = tx->GetHash().GetHex();
            auto it = find_if(pBlock->begin(), pBlock->end(), [&](PTransactionRef const& ptx) { return *ptx == txHash; });
            if (it == pBlock->end())
            {
                LogPrint(BCLog::CONSENSUS, "Warning: SocialConsensus check transaction with type:%d failed with result %d for transaction %s in block %s\n",
                    (int)txType, (int)SocialConsensusResult_PocketDataNotFound, tx->GetHash().GetHex(), block.GetHash().GetHex());

                return {false, SocialConsensusResult_PocketDataNotFound};
            }

            // Check founded payload
            if (auto[ok, result] = check(tx, *it); !ok)
            {
                LogPrint(BCLog::CONSENSUS, "Warning: SocialConsensus check transaction with type:%d failed with result %d for transaction %s in block %s\n",
                    (int)txType, (int)result, tx->GetHash().GetHex(), block.GetHash().GetHex());

                return {false, result};
            }
        }

        return {true, SocialConsensusResult_Success};
    }

    // Проверяет транзакцию без привязки к цепи
    tuple<bool, SocialConsensusResult> SocialConsensusHelper::Check(const CTransactionRef& tx, const PTransactionRef& ptx)
    {
        // Not double check for already in DB
        if (TransRepoInst.Exists(*ptx->GetHash()))
            return {true, SocialConsensusResult_Success};

        if (auto[ok, result] = check(tx, ptx); !ok)
        {
            LogPrint(BCLog::CONSENSUS, "Warning: SocialConsensus %d check failed with result %d for transaction %s\n",
                (int) *ptx->GetType(), (int)result, *ptx->GetHash());

            return {false, result};
        }

        return {true, SocialConsensusResult_Success};
    }

    // -----------------------------------------------------------------

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::check(const CTransactionRef& tx, const PTransactionRef& ptx)
    {
        if (!isConsensusable(*ptx->GetType()))
            return {true, SocialConsensusResult_Success};

        // Check transactions with consensus logic
        tuple<bool, SocialConsensusResult> result;
        switch (*ptx->GetType())
        {
            case ACCOUNT_SETTING:
                return m_accountSettingFactory.Instance(0)->Check(tx, static_pointer_cast<AccountSetting>(ptx));
                break;
            case ACCOUNT_USER:
                return m_userFactory.Instance(0)->Check(tx, static_pointer_cast<User>(ptx));
                break;
            case CONTENT_POST:
                return m_postFactory.Instance(0)->Check(tx, static_pointer_cast<Post>(ptx));
                break;
            case CONTENT_VIDEO:
                return m_videoFactory.Instance(0)->Check(tx, static_pointer_cast<Video>(ptx));
                break;
            case CONTENT_COMMENT:
                return m_commentFactory.Instance(0)->Check(tx, static_pointer_cast<Comment>(ptx));
                break;
            case CONTENT_COMMENT_EDIT:
                return m_commentEditFactory.Instance(0)->Check(tx, static_pointer_cast<CommentEdit>(ptx));
                break;
            case CONTENT_COMMENT_DELETE:
                return m_commentDeleteFactory.Instance(0)->Check(tx, static_pointer_cast<CommentDelete>(ptx));
                break;
            case CONTENT_DELETE:
                return m_contentDeleteFactory.Instance(0)->Check(tx, static_pointer_cast<ContentDelete>(ptx));
                break;
            case ACTION_SCORE_CONTENT:
                return m_scoreContentFactory.Instance(0)->Check(tx, static_pointer_cast<ScoreContent>(ptx));
                break;
            case ACTION_SCORE_COMMENT:
                return m_scoreCommentFactory.Instance(0)->Check(tx, static_pointer_cast<ScoreComment>(ptx));
                break;
            case ACTION_SUBSCRIBE:
                return m_subscribeFactory.Instance(0)->Check(tx, static_pointer_cast<Subscribe>(ptx));
                break;
            case ACTION_SUBSCRIBE_PRIVATE:
                return m_subscribePrivateFactory.Instance(0)->Check(tx, static_pointer_cast<SubscribePrivate>(ptx));
                break;
            case ACTION_SUBSCRIBE_CANCEL:
                return m_subscribeCancelFactory.Instance(0)->Check(tx, static_pointer_cast<SubscribeCancel>(ptx));
                break;
            case ACTION_BLOCKING:
                return m_blockingFactory.Instance(0)->Check(tx, static_pointer_cast<Blocking>(ptx));
                break;
            case ACTION_BLOCKING_CANCEL:
                return m_blockingCancelFactory.Instance(0)->Check(tx, static_pointer_cast<BlockingCancel>(ptx));
                break;
            case ACTION_COMPLAIN:
                return m_complainFactory.Instance(0)->Check(tx, static_pointer_cast<Complain>(ptx));
            // TODO (brangr): future realize types
            case ACCOUNT_VIDEO_SERVER:
            case ACCOUNT_MESSAGE_SERVER:
            case CONTENT_TRANSLATE:
            case CONTENT_SERVERPING:
            default:
                return {true, SocialConsensusResult_Success};
        }
    }

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::validate(const PTransactionRef& ptx, const PocketBlockRef& block, int height)
    {
        if (!isConsensusable(*ptx->GetType()))
            return {true, SocialConsensusResult_Success};

        // Validate transactions with consensus logic
        tuple<bool, SocialConsensusResult> result;
        switch (*ptx->GetType())
        {
            case ACCOUNT_SETTING:
                return m_accountSettingFactory.Instance(height)->Validate(static_pointer_cast<AccountSetting>(ptx), block);
            case ACCOUNT_USER:
                return m_userFactory.Instance(height)->Validate(static_pointer_cast<User>(ptx), block);
            case CONTENT_POST:
                return m_postFactory.Instance(height)->Validate(static_pointer_cast<Post>(ptx), block);
            case CONTENT_VIDEO:
                return m_videoFactory.Instance(height)->Validate(static_pointer_cast<Video>(ptx), block);
            case CONTENT_COMMENT:
                return m_commentFactory.Instance(height)->Validate(static_pointer_cast<Comment>(ptx), block);
            case CONTENT_COMMENT_EDIT:
                return m_commentEditFactory.Instance(height)->Validate(static_pointer_cast<CommentEdit>(ptx), block);
            case CONTENT_COMMENT_DELETE:
                return m_commentDeleteFactory.Instance(height)->Validate(static_pointer_cast<CommentDelete>(ptx), block);
            case CONTENT_DELETE:
                return m_contentDeleteFactory.Instance(height)->Validate(static_pointer_cast<ContentDelete>(ptx), block);
            case ACTION_SCORE_CONTENT:
                return m_scoreContentFactory.Instance(height)->Validate(static_pointer_cast<ScoreContent>(ptx), block);
            case ACTION_SCORE_COMMENT:
                return m_scoreCommentFactory.Instance(height)->Validate(static_pointer_cast<ScoreComment>(ptx), block);
            case ACTION_SUBSCRIBE:
                return m_subscribeFactory.Instance(height)->Validate(static_pointer_cast<Subscribe>(ptx), block);
            case ACTION_SUBSCRIBE_PRIVATE:
                return m_subscribePrivateFactory.Instance(height)->Validate(static_pointer_cast<SubscribePrivate>(ptx), block);
            case ACTION_SUBSCRIBE_CANCEL:
                return m_subscribeCancelFactory.Instance(height)->Validate(static_pointer_cast<SubscribeCancel>(ptx), block);
            case ACTION_BLOCKING:
                return m_blockingFactory.Instance(height)->Validate(static_pointer_cast<Blocking>(ptx), block);
            case ACTION_BLOCKING_CANCEL:
                return m_blockingCancelFactory.Instance(height)->Validate(static_pointer_cast<BlockingCancel>(ptx), block);
            case ACTION_COMPLAIN:
                return m_complainFactory.Instance(height)->Validate(static_pointer_cast<Complain>(ptx), block);
            // TODO (brangr): future realize types
            case ACCOUNT_VIDEO_SERVER:
            case ACCOUNT_MESSAGE_SERVER:
            case CONTENT_TRANSLATE:
            case CONTENT_SERVERPING:
            default:
                return {true, SocialConsensusResult_Success};
        }
    }

    bool SocialConsensusHelper::isConsensusable(TxType txType)
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

}

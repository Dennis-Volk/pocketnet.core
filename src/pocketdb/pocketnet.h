//------------------------------------------------------------------------
// Functions for POCKETNET logic
//------------------------------------------------------------------------
#ifndef POCKETNET_H
#define POCKETNET_H

#include <chainparams.h>
#include <primitives/transaction.h>
#include <core_io.h>
#include <sstream>
#include <rpc/rawtransaction.h>

// Antibot limits
enum Limit {
    threshold_reputation,
    threshold_reputation_score,
    threshold_reputation_complains,
    threshold_reputation_blocking,
    threshold_balance,
    trial_post_limit,
    trial_post_edit_limit,
    trial_score_limit,
    trial_complain_limit,
    full_post_limit,
    full_post_edit_limit,
    full_score_limit,
    full_complain_limit,
    change_info_timeout,
    edit_post_timeout,
    max_user_size,
    max_post_size,
    bad_reputation,
    scores_one_to_one,
    scores_one_to_one_over_comment,
    scores_one_to_one_depth,
    trial_comment_limit,
    trial_comment_edit_limit,
    trial_comment_score_limit,
    full_comment_limit,
    full_comment_edit_limit,
    full_comment_score_limit,
    comment_size_limit,
    edit_comment_timeout,
    scores_depth_modify_reputation,
    lottery_referral_depth
};

void FillLimits(const CChainParams& params);

// Get actual limit for current height
int64_t GetActualLimit(Limit type, int height);


// Pocketnet transaction types
#define OR_SCORE "7570766f74655368617265"
#define OR_COMPLAIN "636f6d706c61696e5368617265"
#define OR_POST "7368617265"
#define OR_POSTEDIT "736861726565646974"
#define OR_SUBSCRIBE "737562736372696265"
#define OR_SUBSCRIBEPRIVATE "73756273637269626550726976617465"
#define OR_UNSUBSCRIBE "756e737562736372696265"
#define OR_USERINFO "75736572496e666f"
#define OR_BLOCKING "626c6f636b696e67"
#define OR_UNBLOCKING "756e626c6f636b696e67"

#define OR_COMMENT "636f6d6d656e74"
#define OR_COMMENT_EDIT "636f6d6d656e7445646974"
#define OR_COMMENT_DELETE "636f6d6d656e7444656c657465"
#define OR_COMMENT_SCORE "6353636f7265"

#define OR_VIDEO "766964656f" // Post for video hosting
#define OR_VERIFICATION "766572696669636174696f6e" // User verification post
#define OR_SERVER "736572766572" // Server registration over User
#define OR_SERVER_PUBLISH "7365727665727075626c697368" // Server card over Posts
#define OR_POLL "706f6c6c" // Polling post
#define OR_TRANSLATE "7472616e736c617465" // Post for translating words


// Check transaction type is pocketnet
bool IsPocketTX(const CTxOut& out);
bool IsPocketTX(const CTransaction& tx);
bool IsPocketTX(const CTransactionRef& tx);
std::string PocketTXType(const CTransactionRef& tx);


// Transaction type convert to reindexer table name
bool ConvertOPToTableName(std::string op, std::string& ri_table);


// Checkpoints for blocks
void FillCheckpointsBlocks(const CChainParams& params);
bool IsCheckpointBlock(int height, std::string hash);


// Checkpoints for transactions
void FillCheckpointsTransactions(const CChainParams& params);
bool IsCheckpointTransaction(std::string hash);

bool GetInputAddress(uint256 txhash, int n, std::string& address);
bool GetTransactionData(std::string txid, std::string& address);
bool GetTransactionData(std::string txid, std::string& address, CTransactionRef& tx);

/*
    Find asm string with OP_RETURN for check type transaction
*/
bool FindPocketNetAsmString(const CTransactionRef& tx, std::vector<std::string>& vasm);
bool FindPocketNetAsmString(const CTransactionRef& tx, std::string& asmStr);
/*
    Return table name for transaction if is PocketNET transaction
*/
bool GetPocketnetTXType(const CTransactionRef& tx, std::string& ri_table);
bool IsPocketnetTransaction(const CTransactionRef& tx);
bool IsPocketnetTransaction(const CTransaction& tx);


#endif // POCKETNET_H
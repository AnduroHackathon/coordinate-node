
#include <anduro_deposit.h>
#include <anduro_validator.h>
#include <chainparams.h>
#include <coordinate/coordinate_preconf.h>
#include <core_io.h>
#include <kernel/mempool_entry.h>
#include <kernel/mempool_persist.h>
#include <node/blockstorage.h>
#include <node/mempool_persist_args.h>
#include <policy/rbf.h>
#include <policy/settings.h>
#include <primitives/transaction.h>
#include <rpc/blockchain.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <txmempool.h>
#include <univalue.h>
#include <util/moneystr.h>
#include <util/time.h>
#include <utility>
#include <net.h>

using node::BlockManager;
using node::DEFAULT_MAX_RAW_TX_FEE_RATE;
using node::NodeContext;

static RPCHelpMan sendpreconftransaction()
{
    return RPCHelpMan{
        "sendpreconftransaction",
        "\nSubmit a raw transaction (serialized, hex-encoded) to local node and network.\n"
        "\nThe transaction will be sent unconditionally to all peers, so using sendpreconftransaction\n"
        "for manual rebroadcast may degrade privacy by leaking the transaction's origin, as\n"
        "nodes will normally not rebroadcast non-wallet transactions already in their mempool.\n"
        "\nA specific exception, RPC_TRANSACTION_ALREADY_IN_CHAIN, may throw if the transaction cannot be added to the mempool.\n"
        "\nRelated RPCs: createrawtransaction, signrawtransactionwithkey\n",
        {
            {"hexstring", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hex string of the raw transaction"},
            {"maxfeerate", RPCArg::Type::AMOUNT, RPCArg::Default{FormatMoney(DEFAULT_MAX_RAW_TX_FEE_RATE.GetFeePerK())},
             "Reject transactions whose fee rate is higher than the specified value, expressed in " + CURRENCY_UNIT +
                 "/kvB.\nSet to 0 to accept any fee rate.\n"},
            {"assethexstring", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "The hex string of the asset data"},
        },
        RPCResult{
            RPCResult::Type::STR_HEX, "", "The transaction hash in hex"},
        RPCExamples{
            "\nCreate a transaction\n" + HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\" : \\\"mytxid\\\",\\\"vout\\\":0}]\" \"{\\\"myaddress\\\":0.01}\"") +
            "Sign the transaction, and get back the hex\n" + HelpExampleCli("signrawtransactionwithwallet", "\"myhex\"") +
            "\nSend the transaction (signed hex)\n" + HelpExampleCli("sendrawtransaction", "\"signedhex\"") +
            "\nAs a JSON-RPC call\n" + HelpExampleRpc("sendrawtransaction", "\"signedhex\"")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            CMutableTransaction mtx;
            if (!DecodeHexTx(mtx, request.params[0].get_str())) {
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed. Make sure the tx has at least one input.");
            }
            CTransactionRef tx(MakeTransactionRef(std::move(mtx)));
            const CFeeRate max_raw_tx_fee_rate = request.params[1].isNull() ?
                                                     DEFAULT_MAX_RAW_TX_FEE_RATE :
                                                     CFeeRate(AmountFromValue(request.params[1]));

            int64_t virtual_size = GetVirtualTransactionSize(*tx);
            CAmount max_raw_tx_fee = max_raw_tx_fee_rate.GetFee(virtual_size);
            std::string err_string;
            AssertLockNotHeld(cs_main);
            NodeContext& node = EnsureAnyNodeContext(request.context);
            const CTransaction& ptx = *tx;

            if (ptx.nVersion != TRANSACTION_PRECONF_VERSION) {
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "asset data missing to submit in mempool");
            }

            const TransactionError err = BroadcastTransaction(node, tx, err_string, max_raw_tx_fee, /*relay=*/true, /*wait_callback=*/true, true);
            if (TransactionError::OK != err) {
                throw JSONRPCTransactionError(err, err_string);
            }

            return tx->GetHash().GetHex();
        },
    };
}

static RPCHelpMan sendpreconflist()
{
    return RPCHelpMan{
        "sendpreconflist",
        "\nSubmit a preconf block submission from anduro federation \n",
        {
            {"block", RPCArg::Type::ARR, RPCArg::Optional::NO, "pre signed preconf signature details from anduro", 
            {{
                "",
                RPCArg::Type::OBJ,
                RPCArg::Optional::NO,
                "",
                {
                    {"txid", RPCArg::Type::STR, RPCArg::Optional::NO, "Transaction id which is in preconf mempool"},
                    {"signed_block_height", RPCArg::Type::NUM, RPCArg::Optional::NO, "the block height where the signatures get invalidated. the signature will be deleted after the block height"},
                    {"mined_block_height", RPCArg::Type::NUM, RPCArg::Optional::NO, "the mined block height where the federation is refer to for sig validation"},
                    {"reserve", RPCArg::Type::NUM, RPCArg::Optional::NO, "Transaction reserve amount"},
                    {"vsize", RPCArg::Type::NUM, RPCArg::Optional::NO, "Transaction virtual size"},
                },
            }}},
            {"witness", RPCArg::Type::STR, RPCArg::Optional::NO, "preconf witness for block"},
            {"finalized", RPCArg::Type::STR, RPCArg::Optional::NO, "Finalized list from federation leader or not"},
            {"federationkey", RPCArg::Type::STR, RPCArg::Optional::NO, "Federation identification"},
            {"pegins",RPCArg::Type::ARR, RPCArg::Optional::OMITTED, "pegin list",
              {
                {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                    {
                        {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address which will get deposit as pegin"},
                        {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Pegin amount details"},
                    }
                }
              }
            },
        },
        RPCResult{
            RPCResult::Type::BOOL, "", "the result for preconf block submission from anduro federation"},
        RPCExamples{
            "\nSend the signature for preconf block\n"
            + HelpExampleCli("sendpreconflist", "\"[{\\\"txid\\\":\\\"mytxid\\\",\\\"signed_block_height\\\":0,\\\"mined_block_height\\\":0,\\\"reserve\\\":0,\\\"vsize\\\":0}]\" \"witness\" \"finalized\" \"federationkey\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            NodeContext& node = EnsureAnyNodeContext(request.context);
            ChainstateManager& chainman = EnsureChainman(node);
            if (!chainman.GetParams().IsTestChain()) {
                const CConnman& connman = EnsureConnman(node);
                if (connman.GetNodeCount(ConnectionDirection::Both) == 0) {
                    throw JSONRPCError (RPC_CLIENT_NOT_CONNECTED,
                                    "Coordinate is not connected!");
                }
                if (chainman.IsInitialBlockDownload()) {
                    throw JSONRPCError (RPC_CLIENT_IN_INITIAL_DOWNLOAD,
                                    "Coordinate is downloading blocks...");
                }
            }



            uint32_t finalized = 0;
            if(!request.params[2].isNull()) {
                if(!ParseUInt32(request.params[2].get_str(),&finalized)) {
                    throw JSONRPCError (RPC_CLIENT_IN_INITIAL_DOWNLOAD,
                                    "Error converting block height.");
                }
            }
            const UniValue& req_params = request.params[0].get_array();
            if (req_params.size() == 0) {
                return false;
            }

            std::vector<CTxOut> pegins;
            std::vector<CoordinatePreConfSig> preconf;
     

            if(!request.params[4].isNull()) {
                const UniValue& pegin_params = request.params[4].get_array();
                   if (pegin_params.size() > 0) {
                        for (unsigned int idx = 0; idx < pegin_params.size(); idx++) {
                            const UniValue& peginParams = pegin_params[idx].get_obj();
                            RPCTypeCheckObj(peginParams,
                            {
                                {"address", UniValueType(UniValue::VSTR)},
                                {"amount", UniValueType(UniValue::VNUM)},
                            });
                            const CTxDestination coinbaseScript = DecodeDestination(peginParams.find_value("address").get_str());
                            const CScript scriptPubKey = GetScriptForDestination(coinbaseScript);
                            CAmount peg_amount = CAmount(peginParams.find_value("amount").getInt<int64_t>());
                            CTxOut pegin(peg_amount, scriptPubKey);
                            pegins.push_back(pegin);
                        }
                   }
            }
 
            CoordinatePreConfSig preconfObj;
            for (unsigned int idx = 0; idx < req_params.size(); idx++) {
                const UniValue& fedParams = req_params[idx].get_obj();
                RPCTypeCheckObj(fedParams,
                                {
                                    {"txid", UniValueType(UniValue::VSTR)},
                                    {"signed_block_height", UniValueType(UniValue::VNUM)},
                                    {"mined_block_height", UniValueType(UniValue::VNUM)},
                                    {"vsize", UniValueType(UniValue::VNUM)},
                                    {"reserve", UniValueType(UniValue::VNUM)},
                                });
                std::string receivedTx = fedParams.find_value("txid").get_str();
               
                if (receivedTx.compare("") != 0) {
                    preconfObj.txids.push_back(uint256S(receivedTx));
                } else {

                    preconfObj.txids.push_back(uint256::ZERO);
                }
                preconfObj.blockHeight =  fedParams.find_value("signed_block_height").getInt<int64_t>();
                preconfObj.minedBlockHeight =  fedParams.find_value("mined_block_height").getInt<int64_t>();
                preconfObj.finalized = finalized;
                preconfObj.witness =  request.params[1].get_str();
                preconfObj.federationKey = request.params[3].get_str();
                if(idx == 0 && pegins.size() > 0) {
                    preconfObj.pegins = pegins;
                }
                
            }
            preconf.push_back(preconfObj);
            return includePreConfSigWitness(preconf, chainman);
    

            return false;
        },
    };
}

static RPCHelpMan getpreconffee()
{
    return RPCHelpMan{
        "getpreconffee",
        "\nGet current preconfirmation fee \n",
        {},
        RPCResult{
            RPCResult::Type::NUM, "", "current preconf minimum fee"},
        RPCExamples{
            "\nGet current transaciton fee\n" + HelpExampleCli("sendpreconfsignatures", "")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            return getPreConfMinFee();
        },
    };
}

static RPCHelpMan getpreconflist()
{
    return RPCHelpMan{
        "getpreconflist",
        "\nGet current getpreconf votes list \n",
        {
            {"height", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "The height index"},
        },
        RPCResult{
            RPCResult::Type::OBJ,
            "",
            "",
            {
                {RPCResult::Type::ARR, "block", "",
                {
                     {RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::STR, "txid", "preconf transaction txid"},
                            {RPCResult::Type::NUM, "mined_block_height", "mine block height"},
                            {RPCResult::Type::NUM, "signed_block_height", "signed block height"},
                            {RPCResult::Type::NUM, "reserve", "Transaction reserve amount"},
                            {RPCResult::Type::NUM, "vsize", "Transaction virtual size"},
                            {RPCResult::Type::NUM, "finalized", "Finalized list from federation leader or not"},
                            {RPCResult::Type::STR, "federationkey", "federation identification"}
                        }
                     }
                }},
            },
        },
        RPCExamples{"\nGet current preconf block in queue\n" + HelpExampleCli("getpreconflist", "height")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {

            const NodeContext& node{EnsureAnyNodeContext(request.context)};
            ChainstateManager& chainman = EnsureChainman(node);
            CTxMemPool& preconf_pool{*chainman.ActiveChainstate().GetPreConfMempool()};

            uint32_t nHeight = 0;
            if (!request.params[0].isNull()) {
                if(!ParseUInt32(request.params[0].get_str(), &nHeight)) {
                    throw JSONRPCError (RPC_CLIENT_IN_INITIAL_DOWNLOAD,
                                    "Error converting block height.");
                }
            }

            UniValue result(UniValue::VOBJ);
            UniValue block(UniValue::VARR);
            std::vector<CoordinatePreConfSig> coordinatePreConfSigs = getPreConfSig();
            for (const CoordinatePreConfSig& coordinatePreConfSig : coordinatePreConfSigs) {
                    if (coordinatePreConfSig.blockHeight == (int32_t)nHeight || nHeight == 0) {
                        for (size_t i = 0; i < coordinatePreConfSig.txids.size(); i++)
                        {
                            UniValue voteItem(UniValue::VOBJ);
                            if(coordinatePreConfSig.txids[i] == uint256::ZERO) {
                                voteItem.pushKV("txid","");
                                voteItem.pushKV("reserve", 0);
                                voteItem.pushKV("vsize", 0);
                            } else {
                                TxMempoolInfo info = preconf_pool.info(GenTxid::Txid(coordinatePreConfSig.txids[i]));
                                if(info.tx) {
                                    voteItem.pushKV("txid",  coordinatePreConfSig.txids[i].ToString());
                                    voteItem.pushKV("reserve", info.tx->vout[0].nValue);
                                    voteItem.pushKV("vsize", info.vsize);
                                } else {
                                    voteItem.pushKV("txid","");
                                    voteItem.pushKV("reserve", 0);
                                    voteItem.pushKV("vsize", 0);
                                }
                            }
                                voteItem.pushKV("mined_block_height", coordinatePreConfSig.minedBlockHeight);
                                voteItem.pushKV("signed_block_height", coordinatePreConfSig.blockHeight);
                                voteItem.pushKV("finalized", coordinatePreConfSig.finalized);
                                voteItem.pushKV("federationkey", coordinatePreConfSig.federationKey);
                                block.push_back(voteItem);
                        }

              
                }
            }
            result.pushKV("block", block);
            return result;
        },
    };
}

static RPCHelpMan getfinalizedsignedblocks() {
        return RPCHelpMan{
        "getfinalizedsignedblocks" ,
        "get finalized signed blocks",
        {
        },
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {
                    {RPCResult::Type::OBJ, "", "Signed block details",
                    {
                        {RPCResult::Type::NUM, "fee", "Signed block fee"},
                        {RPCResult::Type::NUM, "blockindex", "block index where anduro witness refer back to the pubkeys"},
                        {RPCResult::Type::NUM, "height", "Signed block height"},
                        {RPCResult::Type::NUM, "time", "Signed block time"},
                        {RPCResult::Type::STR_HEX, "previousblock", "previous signed block hash"},
                        {RPCResult::Type::STR_HEX, "merkleroot", "signed block merkle root hash"},
                        {RPCResult::Type::STR_HEX, "hash", "Signed block hash"},
                        {RPCResult::Type::ARR, "tx", "The transaction ids",
                            {{RPCResult::Type::STR_HEX, "", "The transaction id"}}},
                    }},
                },
            },
        },
        RPCExamples{
            HelpExampleCli("getfinalizedsignedblocks", ""),

        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            LOCK(cs_main);
            UniValue result(UniValue::VARR);
            std::vector<SignedBlock> finalizedBlocks = getFinalizedSignedBlocks();
            for (const SignedBlock& block : finalizedBlocks) {
                int blockSize = 0;
                blockSize += sizeof(block.currentFee);
                blockSize += sizeof(block.blockIndex);
                blockSize += sizeof(block.nHeight);
                blockSize += sizeof(block.nTime);
                blockSize += block.hashPrevSignedBlock.size();
                blockSize += block.hashMerkleRoot.size();
                blockSize += block.GetHash().size();
                for (const CTransactionRef& tx : block.vtx) {
                    blockSize +=  GetVirtualTransactionSize(*tx);
                }

                UniValue blockDetails(UniValue::VOBJ); 
                UniValue txs(UniValue::VARR);
                for (const CTransactionRef& tx : block.vtx) {
                    txs.push_back(tx->GetHash().ToString());
                }

                blockDetails.pushKV("fee", block.currentFee);
                blockDetails.pushKV("blockindex", (uint64_t)block.blockIndex);
                blockDetails.pushKV("height", (uint64_t)block.nHeight);
                blockDetails.pushKV("time", block.nTime);
                blockDetails.pushKV("size", blockSize);
                blockDetails.pushKV("previousblock", block.hashPrevSignedBlock.ToString());
                blockDetails.pushKV("merkleroot", block.hashMerkleRoot.ToString());
                blockDetails.pushKV("hash", block.GetHash().ToString());
                blockDetails.pushKV("tx", txs);

                result.push_back(blockDetails); 
            }

            return result;
        }
    };
}

static RPCHelpMan getsignedblockcount()
{
    return RPCHelpMan{
        "getsignedblockcount",
        "\nReturns the height of the signed block count\n"
        "The genesis block has height 1.\n",
        {},
        RPCResult{
            RPCResult::Type::NUM, "", "The current signed block count"},
        RPCExamples{
            HelpExampleCli("getsignedblockcount", "") + HelpExampleRpc("getsignedblockcount", "")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            ChainstateManager& chainman = EnsureAnyChainman(request.context);
            LOCK(cs_main);
            uint64_t nHeight = 0;
            chainman.ActiveChainstate().psignedblocktree->GetLastSignedBlockID(nHeight);
            return nHeight;
        },
    };
}


static RPCHelpMan getpreconftxrefund() {
    return RPCHelpMan{
        "getpreconftxrefund",
        "\nGet preconf tx refund\n",
        {
            {"txs",RPCArg::Type::ARR, RPCArg::Optional::NO, "preconf tx list",
              {
                {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                    {
                        {"tx", RPCArg::Type::STR, RPCArg::Optional::NO, "preconf tx"},
                    }
                }
              }
            },
        },
        RPCResult{
            RPCResult::Type::ARR, "", "", {
                {
                    {RPCResult::Type::OBJ, "", "preconf tx details",
                    {
                        {RPCResult::Type::STR, "tx", "preconf tx"},
                        {RPCResult::Type::NUM, "refund", "preconf tx refund"},
                    }},
                },
            },
        },
        RPCExamples{
            HelpExampleCli("getpreconftxrefund", "") + HelpExampleRpc("getpreconftxrefund", "")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            const UniValue& req_params = request.params[0].get_array();
                   
            if (req_params.size() == 0) {
               throw JSONRPCError (RPC_CLIENT_IN_INITIAL_DOWNLOAD,
                                    "Txid not available");
            }
            LOCK(cs_main);
            const NodeContext& node = EnsureAnyNodeContext(request.context);
            ChainstateManager& chainman = EnsureChainman(node);
            CCoinsViewCache view(&chainman.ActiveChainstate().CoinsTip());
            chainman.ActiveChainstate().UpdatedCoinsTip(view,chainman.ActiveChainstate().m_chain.Height());


            const CBlockIndex* blockindex = nullptr;
            uint256 hash_block;
            UniValue result(UniValue::VARR);  
            for (unsigned int idx = 0; idx < req_params.size(); idx++) {
                const UniValue& params = req_params[idx].get_obj();
                RPCTypeCheckObj(params,{
                    {"tx", UniValueType(UniValue::VSTR)},
                });
                CAmount refund = CAmount(0);
                uint256 hash = ParseHashV(params.find_value("tx"), "parameter 1");
                CTransactionRef mined_tx;
                std::vector<SignedBlock> finalizedBlocks= getFinalizedSignedBlocks();
                for (SignedBlock finalizedSignedBlock : finalizedBlocks) {
                    for (const auto& tx : finalizedSignedBlock.vtx) {
                        if (tx->GetHash() == hash) {
                            mined_tx =  tx;
                            refund = getRefundForPreconfCurrentTx(*mined_tx,finalizedSignedBlock.currentFee,view);
                            break;
                        }
                    }
                }
                if(!mined_tx) {
                    SignedTxindex signedTxIndex;
                    chainman.ActiveChainstate().psignedblocktree->getTxPosition(hash,signedTxIndex);
                    if(!signedTxIndex.signedBlockHash.IsNull()) {
                        CChain& active_chain = chainman.ActiveChain();
                        CBlock block;
                        if (chainman.m_blockman.ReadBlockFromDisk(block, *active_chain[signedTxIndex.blockIndex])) {
                            for (const SignedBlock& preconfBlockItem : block.preconfBlock) {
                                if(preconfBlockItem.GetHash() == signedTxIndex.signedBlockHash) {
                                    mined_tx = preconfBlockItem.vtx[signedTxIndex.pos];
                                    refund = getRefundForPreconfCurrentTx(*mined_tx,preconfBlockItem.currentFee,view);
                                    break;
                                }
                            }
                        } 
                    }
                }
                UniValue preconfDetails(UniValue::VOBJ); 
                preconfDetails.pushKV("tx", hash.ToString());
                preconfDetails.pushKV("refund", refund);
                result.push_back(preconfDetails); 
            }
            return result;
        },
    };
}


void RegisterPreConfMempoolRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[]{
        {"preconf", &sendpreconftransaction},
        {"preconf", &sendpreconflist},
        {"preconf", &getpreconffee},
        {"preconf", &getpreconflist},
        {"preconf", &getfinalizedsignedblocks},
        {"preconf", &getsignedblockcount},
        {"preconf", &getpreconftxrefund}
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}

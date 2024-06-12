// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <logging.h>
#include <univalue.h>
#include <node/blockstorage.h>
#include <rpc/util.h>
#include <rpc/client.h>
#include <anduro_deposit.h>
#include <anduro_validator.h>

using node::BlockManager;
using node::ReadBlockFromDisk;
// temporary storage for including presigned signature on next block
std::vector<AnduroTxOut> tDeposits;
// check blocks are fully synced to active anduro presign validation
bool isValidationActivate = false;
// temporary storage for deposit address
std::string depositAddress = "";
// temporary storage for withdraw address
std::string burnAddress = "";

/**
 * Include presigned signature from anduro.
 */
void includePreSignedSignature(std::vector<AnduroTxOut> txOuts) {
   if (txOuts.size() == 0) {
      return;
   }
   if(!isSignatureAlreadyExist(txOuts[0])) {
      for (const AnduroTxOut& tx : txOuts) {
            tDeposits.push_back(tx);
            depositAddress = tx.depositAddress;
            burnAddress = tx.burnAddress;
      }
   }
}

bool isSignatureAlreadyExist(AnduroTxOut txOut) {
   bool isExist = false;
   std::vector<AnduroTxOut> pending_deposits = listPendingDepositTransaction(txOut.block_height);
   if(pending_deposits.size() > 0) {
      isExist = true;
   }
   return isExist;
}

bool isSpecialTxoutValid(std::vector<AnduroTxOut> txOuts, ChainstateManager& chainman) {

   if(txOuts.size()==0) {
      return false;
   }
   LOCK(cs_main);
   CChain& active_chain = chainman.ActiveChain();
   int blockindex = active_chain.Height();
   if(txOuts[0].block_height <= blockindex) {
      blockindex = blockindex - 1;
   }
   // get block to find the eligible anduro keys to be signed on presigned block
   CBlock block;
   if (!ReadBlockFromDisk(block, CHECK_NONFATAL(active_chain[blockindex]), Params().GetConsensus())) {
        LogPrintf("Error reading block from disk at index %d\n", CHECK_NONFATAL(active_chain[blockindex])->GetBlockHash().ToString());
   }

   UniValue messages(UniValue::VARR);
   int tIndex = 1;
   // preparing message for signature verification
   for (const AnduroTxOut& txOut : txOuts) {

      CTxDestination address;
      ExtractDestination(txOut.scriptPubKey, address);
      std::string addressStr = EncodeDestination(address);

      UniValue message(UniValue::VOBJ);
      message.pushKV("address", addressStr);
      message.pushKV("amount", txOut.nValue);
      message.pushKV("index", tIndex);
      message.pushKV("height", txOut.block_height);

      tIndex = tIndex + 1;
      messages.push_back(message);
   }
   // check signature is valid
   bool isValid = validateAnduroSignature(txOuts[0].witness,messages.write(),block.currentKeys);

   if (isValid) {
      return true;
   }

   return false;
}

/**
 * This function list all presigned pegin details for upcoming blocks by height
 */
std::vector<AnduroTxOut> listPendingDepositTransaction(uint32_t block_height) {
    if(block_height == -1) {
        return tDeposits;
    }

    std::vector<AnduroTxOut> tDepositsNew;
    for (const AnduroTxOut& tx_out : tDeposits) {
        if(tx_out.block_height == block_height) {
            tDepositsNew.push_back(tx_out);
        }
    }

    return tDepositsNew;
}

/**
 * This function find total pegin amount for particular block
 */
CAmount listPendingDepositTotal(uint32_t block_height) {
    std::vector<AnduroTxOut> tDepositsNew;
    if(block_height == -1) {
        tDepositsNew = tDeposits;
    } else {
        for (const AnduroTxOut& tx_out : tDeposits) {
            if(tx_out.block_height == block_height) {
                tDepositsNew.push_back(tx_out);
            }
        }
    }

    CAmount totalDeposit = CAmount(0);
    for (const AnduroTxOut& txOut: tDepositsNew) {
        totalDeposit = totalDeposit + CAmount(txOut.nValue);
    }

    return totalDeposit;
}

/**
 * Used to reset presigned signature for processed blocks
 */
void resetDeposit(uint32_t block_height) {
    std::vector<AnduroTxOut> tDepositsNew;
      for (const AnduroTxOut& tx_out : tDeposits) {
         if(tx_out.block_height > block_height) {
               tDepositsNew.push_back(tx_out);
         }
      }
    tDeposits = tDepositsNew;
}

/**
 * Used to get current keys to be signed for upcoming block
 */
std::string getCurrentKeys(ChainstateManager& chainman) {
   int block_index = chainman.ActiveChain().Height();
   LOCK(cs_main);
   CChain& active_chain = chainman.ActiveChain();
   CBlock block;
   if (!ReadBlockFromDisk(block, CHECK_NONFATAL(active_chain[block_index]), Params().GetConsensus())) {
        // Log the disk read error to the user.
        LogPrintf("Error reading block from disk at index %d\n", CHECK_NONFATAL(active_chain[block_index])->GetBlockHash().ToString());
   }
   return block.currentKeys;
}

/**
 * Used to get current next index to be signed for upcoming block
 */
int32_t getNextIndex(ChainstateManager& chainman) {
   int blockindex = chainman.ActiveChain().Height();
   LOCK(cs_main);
   CChain& active_chain = chainman.ActiveChain();
   CBlock block;
   if (!ReadBlockFromDisk(block, CHECK_NONFATAL(active_chain[blockindex]), Params().GetConsensus())) {
        // Log the disk read error to the user.
        LogPrintf("Error reading block from disk at index %d\n", CHECK_NONFATAL(active_chain[blockindex])->GetBlockHash().ToString());
   }
   return block.nextIndex;
}

/**
 * Check block are fully synced to start validating anduro new presigned signature for upcoming blocks
 */
bool isAnduroValidationActive() {
   return isValidationActivate;
}

/**
 * Validate the anduro signature on confirmed blocks
 */
bool verifyAnduro(ChainstateManager& chainman, const CBlock& block) {
   LOCK(cs_main);
   CChain& active_chain = chainman.ActiveChain();
   // activate presigned signature checker after blocks fully synced in node
   if(listPendingDepositTransaction(active_chain.Height()+1).size()>0) {
         isValidationActivate = true;
   }

   if(!isAnduroValidationActive()) {
      return true;
   }

   // check coinbase should have three output
   //  0 - fee reward for merge mine
   //  1 - coinbase message
   //  2 - signature by previous block anduro current keys
   if(block.vtx[0]->vout.size() < 3) {
      return false;
   }

   // check for current keys for anduro
   CBlock prevblock;
   if (!ReadBlockFromDisk(prevblock, CHECK_NONFATAL(active_chain[active_chain.Height()]), Params().GetConsensus())) {
      return false;
   }

   std::vector<unsigned char> wData(ParseHex(prevblock.currentKeys));
   const std::string prevWitnessHexStr(wData.begin(), wData.end());
   UniValue witnessVal(UniValue::VOBJ);
   if (!witnessVal.read(prevWitnessHexStr)) {
      LogPrintf("invalid witness params \n");
      return false;
   }

   CTxOut witnessOut = block.vtx[0]->vout[block.vtx[0]->vout.size()-2];

   const std::string witnessStr = ScriptToAsmStr(witnessOut.scriptPubKey).replace(0,10,"");

   std::vector<AnduroTxOut> tOuts;
   if(block.vtx[0]->vout.size() == 3) {
      const CTxDestination coinbaseScript = DecodeDestination(find_value(witnessVal.get_obj(), "current_address").get_str());
      const CScript scriptPubKey = GetScriptForDestination(coinbaseScript);
      AnduroTxOut out(AmountFromValue(0), scriptPubKey, witnessStr, active_chain.Height() + 1,block.nextIndex,block.currentKeys, "", "");
      tOuts.push_back(out);
   } else {
      // if more than 3 output in coinbase should be considered as pegin and recreating presigned signature for pegin to verify with anduro current keys
      for (size_t i = 1; i < block.vtx[0]->vout.size()-2; i=i+1) {
         CTxOut pegTx = block.vtx[0]->vout[i];
         AnduroTxOut out(pegTx.nValue, pegTx.scriptPubKey, witnessStr, active_chain.Height() + 1,block.nextIndex,block.currentKeys, "", "");
         tOuts.push_back(out);
      }
   }

   if(!isSpecialTxoutValid(tOuts,chainman)) {
      return false;
   }

   return true;
}

/**
 * Get recent bitcoin deposit address used to peg in
 */
std::string getDepositAddress() {
   return depositAddress;
}

/**
 * Get sidechain withdrawal address used to peg out
 */
std::string getBurnAddress() {
   return burnAddress;
}

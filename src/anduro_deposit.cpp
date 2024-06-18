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
// temporary storage for including precommitment signature on next block
std::vector<AnduroPreCommitment> tCommitments;
// check blocks are fully synced to active anduro presign validation
bool isValidationActivate = false;
// temporary storage for deposit address
std::string depositAddress = "";
// temporary storage for withdraw address
std::string burnAddress = "";

/**
 * Include precommitment signature from anduro.
 */
void includePreCommitmentSignature(std::vector<AnduroPreCommitment> commitments) {
   if (commitments.size() == 0) {
      return;
   }
   if(!isSignatureAlreadyExist(commitments[0])) {
      for (const AnduroPreCommitment& commitment : commitments) {
            tCommitments.push_back(commitment);
            depositAddress = commitment.depositAddress;
            burnAddress = commitment.burnAddress;
      }
   }
}

bool isSignatureAlreadyExist(AnduroPreCommitment commitment) {
   bool isExist = false;
   std::vector<AnduroPreCommitment> pending_commitments = listPendingCommitment(commitment.block_height);
   if(pending_commitments.size() > 0) {
      isExist = true;
   }
   return isExist;
}

bool isPreCommitmentValid(std::vector<AnduroPreCommitment> commitments, ChainstateManager& chainman) {

   if(commitments.size()==0) {
      LogPrintf("isPreCommitmentValid no size");
      return false;
   }
   LOCK(cs_main);
   CChain& active_chain = chainman.ActiveChain();

   bool isValid = true;
   
   UniValue messages(UniValue::VARR);
   // preparing message for signature verification
   for (const AnduroPreCommitment& commitment : commitments) {
      if(commitment.block_height <= active_chain.Height()) {
         LogPrintf("precommitment witness hold old block information \n");
         isValid = false;
         break;
      }
      int blockindex = commitment.block_height - 3;
      
      if (blockindex < 0) {
         LogPrintf("precommitment witness hold invalid block information\n");
         isValid = false;
         break;
      }
      // get block to find the eligible anduro keys to be signed on precommitment block
      CBlock block;
      if (!chainman.m_blockman.ReadBlockFromDisk(block, *CHECK_NONFATAL(active_chain[blockindex]))) {
         LogPrintf("Error reading block from disk at index %d\n", CHECK_NONFATAL(active_chain[blockindex])->GetBlockHash().ToString());
      }

      isValid = validateAnduroSignature(commitment.witness,block.GetHash().ToString(),block.currentKeys);
   }

   // check signature is valid
   if (isValid) {
      return true;
   }

   return false;
}

/**
 * This function list all precommitment commitment details for upcoming blocks by height
 */
std::vector<AnduroPreCommitment> listPendingCommitment(int32_t block_height) {
    if(block_height == -1) {
        return tCommitments;
    }
    std::vector<AnduroPreCommitment> tCommitmentsNew;
    for (const AnduroPreCommitment& commitment : tCommitments) {
        if(commitment.block_height == block_height) {
            tCommitmentsNew.push_back(commitment);
        }
    }
    return tCommitmentsNew;
}

/**
 * Used to reset precommitment signature for processed blocks
 */
void resetCommitment(int32_t block_height) {
   std::vector<AnduroPreCommitment> tCommitmentsNew;
   for (const AnduroPreCommitment& commitment : tCommitments) {
      if(commitment.block_height > block_height) {
         tCommitmentsNew.push_back(commitment);
      }
   }
    tCommitments = tCommitmentsNew;
}

/**
 * Used to get current keys to be signed for upcoming block
 */
std::string getCurrentKeys(ChainstateManager& chainman) {
   LOCK(cs_main);
   int block_index = chainman.ActiveChain().Height();
   CChain& active_chain = chainman.ActiveChain();
   CBlock block;
   if (!chainman.m_blockman.ReadBlockFromDisk(block, *CHECK_NONFATAL(active_chain[block_index]))) {
        // Log the disk read error to the user.
        LogPrintf("Error reading block from disk at index %d\n", CHECK_NONFATAL(active_chain[block_index])->GetBlockHash().ToString());
   }
   return block.currentKeys;
}

/**
 * Used to get current next index to be signed for upcoming block
 */
int32_t getCurrentIndex(ChainstateManager& chainman) {
   LOCK(cs_main);
   int blockindex = chainman.ActiveChain().Height();
   CChain& active_chain = chainman.ActiveChain();
   CBlock block;
   if (!chainman.m_blockman.ReadBlockFromDisk(block, *CHECK_NONFATAL(active_chain[blockindex]))) {
        // Log the disk read error to the user.
        LogPrintf("Error reading block from disk at index %d\n", CHECK_NONFATAL(active_chain[blockindex])->GetBlockHash().ToString());
   }
   return block.currentIndex;
}

/**
 * Check block are fully synced to start validating anduro new precommitment signature for upcoming blocks
 */
bool isAnduroValidationActive() {
   return isValidationActivate;
}

/**
 * Validate the anduro signature on confirmed blocks
 */
bool verifyPreCommitment(ChainstateManager& chainman, const CBlock& block, int currentHeight) {
   if(chainman.GetParams().GetChainType() == ChainType::REGTEST) {
      return true;
   }

   LOCK(cs_main);
   if(currentHeight < 3) {
      LogPrintf("verifyCommitment: gensis block ignored");
      return true;
   }
   CChain& active_chain = chainman.ActiveChain();
   // activate precommitment signature checker after blocks fully synced in node
   if(listPendingCommitment(currentHeight).size()>0) {
         isValidationActivate = true;
   }

   if(!isAnduroValidationActive()) {
      return true;
   }

   // check coinbase should have five output
   //  0 - fee reward for merge mine
   //  1 - preconf miner script
   //  2 - federation script
   //  3 - signature by previous block anduro current keys
   //  4 - coinbase message

   if(block.vtx[0]->vout.size() < 4) {
      return false;
   }

   // check for current keys for anduro
   CBlock prevblock;
   if (!chainman.m_blockman.ReadBlockFromDisk(prevblock, *CHECK_NONFATAL(active_chain[currentHeight - 3]))) {
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

   std::vector<AnduroPreCommitment> commitments;
   AnduroPreCommitment out(witnessStr,currentHeight,block.currentIndex,block.currentKeys, "", "");
   commitments.push_back(out);

   if(!isPreCommitmentValid(commitments,chainman)) {
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

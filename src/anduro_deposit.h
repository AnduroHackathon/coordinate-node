// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_FEDERAITON_H
#define BITCOIN_FEDERAITON_H

#include <iostream>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <validation.h>
#include <key_io.h>
#include <primitives/transaction.h>
#include <logging.h>
#include <util/message.h>

/**
 *  This is call is used to receive or prepare anduro precommitment signature data along with anduro deposit and withdrawal address
 */
class AnduroPreCommitment
{
public:
    std::string witness;  /*!< precommitment signature information  */
    int32_t block_height; /*!< block height where the precommitment signature belong to */
    std::string nextKeys; /*!< next key going to be included in block header */
    int32_t nextIndex; /*!< next derivation index going to be include in block header. This index is refer back by anduro and identify the  current derivation path to be signed*/
    std::string depositAddress; /*!< hold current deposit address from bitcoin */
    std::string burnAddress; /*!< hold current withdrawal address from sidechain */

    AnduroPreCommitment()
    {
        SetNull();
    }

    /**
     * @param[in] witnessesIn       precommitment signature information
     * @param[in] blockHeightIn     precommitment signature block height
     * @param[in] nextIndexIn       next derivation index going to be include in block header
     * @param[in] nextKeysIn        next key going to be included in block header
     * @param[in] depositAddressIn  hold current deposit address from bitcoin
     * @param[in] burnAddressIn     hold current withdrawal address from sidechain
     */
    AnduroPreCommitment(std::string witnessesIn, int32_t blockHeightIn, int32_t nextIndexIn, std::string nextKeysIn, std::string depositAddressIn, std::string burnAddressIn) {
        witness = witnessesIn;
        block_height = blockHeightIn;
        nextIndex = nextIndexIn;
        nextKeys = nextKeysIn;
        depositAddress = depositAddressIn;
        burnAddress = burnAddressIn;
    }

    SERIALIZE_METHODS(AnduroPreCommitment, obj) {
        READWRITE(obj.witness, obj.block_height, obj.nextKeys, obj.nextIndex, obj.depositAddress, obj.burnAddress);
    }

    void SetNull()
    {
        witness = "";
        block_height = 0;
        nextKeys = "";
        nextIndex = 0;
        depositAddress = "";
        burnAddress = "";
    }
};

/**
 * This function get recent bitcoin deposit address used to peg in
 */
std::string getDepositAddress();

/**
 * This function get sidechain withdrawal address used to peg out
 */
std::string getBurnAddress();

/**
 * This function include precommitments signature from anduro
 * @param[in] commitments hold three block precommitment signature.
 */
void includePreCommitmentSignature(std::vector<AnduroPreCommitment> commitments);

/**
 * This function check block are fully synced to start validating anduro new precommitment signature for upcoming blocks
 */
bool isAnduroValidationActive();

/**
 * validate the anduro signature on confirmed blocks
 * @param[in] chainman  used to find previous blocks based on active chain state to find anduro current keys
 * @param[in] block  current block received from connectblock
 * @param[in] currentHeight  current height where preconf need to verify
 */
bool verifyPreCommitment(ChainstateManager& chainman, const CBlock& block, int currentHeight);

/**
 * This function check precommitment signatures are valid
 * @param[in] commitments  precommitment signature for validation
 * @param[in] chainman  used to find next block number and active chain state
 */
bool isPreCommitmentValid(std::vector<AnduroPreCommitment> commitments, ChainstateManager& chainman);

/**
 * This function used to check whether precommitment signature already exist when received signature through peer message
 * @param[in] commitments  precommitment signature details
 */
bool isSignatureAlreadyExist(AnduroPreCommitment commitments);

/**
 * This function used to reset presigned signature for processed blocks
 * @param[in] block_height  block height to clear presigned signature
 */
void resetCommitment(int32_t block_height);

/**
 * This function used to get current keys to be signed for upcoming block
 * @param[in] chainman  used to get active chain state
 */
std::string getCurrentKeys(ChainstateManager& chainman);

/**
 * This function used to get current next index to be signed for upcoming block
 * @param[in] chainman  used to get active chain state
 */
int32_t getCurrentIndex(ChainstateManager& chainman);

/**
 * This function list all presigned pegin details for upcoming blocks by height
 * @param[in] block_height  block height to find pending pegin
 */
std::vector<AnduroPreCommitment> listPendingCommitment(int32_t block_height);


#endif // BITCOIN_FEDERAITON_H

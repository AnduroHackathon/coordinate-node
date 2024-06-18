#include <iostream>
#include <util/strencodings.h>
#include "pubkey.h"
#include <secp256k1_schnorrsig.h>
#include <key_io.h>

/**
 * This function check witness signature path available in authorized anduro keys
 * @param[in] fullQuorum  authorized anduro full path
 * @param[in] signaturePath  path signed by the anduro for presigned block
 */
bool getRedeemPathAvailable(std::vector<std::string> fullQuorum, std::string signaturePath);

/**
 * This function check used to prepare sha256 hash for presigned block message
 * @param[in] message  presigned block message
 */
uint256 prepareMessageHash(std::string message);

/**
 * This function used to validate presigned signature
 * @param[in] witnessHex  block witness which hold signature path and signature
 * @param[in] message  sha256 presigned block message
 * @param[in] prevWitnessHex anduro current keys
*/
bool validateAnduroSignature(std::string witnessHex, std::string message, std::string prevWitnessHex);

/**
 * This function used to validate presigned signature
 * @param[in] signatureHex  block witness which hold signature path and signature
 * @param[in] messageIn  sha256 presigned block message
 * @param[in] prevWitnessHex anduro current keys
*/
bool validatePreconfSignature(std::string signatureHex, std::string messageIn, std::string prevWitnessHex);
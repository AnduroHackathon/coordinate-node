#include <anduro_validator.h>
#include <string.h>
#include <logging.h>
#include <outputtype.h>
#include <rpc/util.h>
#include <cmath>
/**
 * Validate presigned signature
 */
bool validateAnduroSignature(std::string signatureHex, std::string messageIn, std::string prevWitnessHex) {
    std::vector<unsigned char> wData(ParseHex(prevWitnessHex));
    const std::string prevWitnessHexStr(wData.begin(), wData.end());
    UniValue witnessVal(UniValue::VOBJ);
    if (!witnessVal.read(prevWitnessHexStr)) {
        LogPrintf("invalid witness params \n");
        return false;
    }

    std::vector<std::string> allKeysArray;
    const auto allKeysArrayRequest = witnessVal.get_obj().find_value("all_keys").get_array();
    for (size_t i = 0; i < allKeysArrayRequest.size(); i++) {
        allKeysArray.push_back(allKeysArrayRequest[i].get_str());
    }
    int thresold =  ((allKeysArray.size()-(allKeysArray.size() % 2))/2) + 1;
    std::vector<unsigned char> sData(ParseHex(signatureHex));
    const std::string signatureHexStr(sData.begin(), sData.end());
    UniValue allSignatures(UniValue::VARR);
    if (!allSignatures.read(signatureHexStr)) {
        LogPrintf("invalid signature params \n");
        return false;
    }
    for (unsigned int idx = 0; idx < allSignatures.size(); idx++) {
        const UniValue& o = allSignatures[idx].get_obj();
        RPCTypeCheckObj(o,
        {
            {"redeempath", UniValueType(UniValue::VSTR)},
            {"signature", UniValueType(UniValue::VSTR)},
        });
        std::string redeemPath =  o.find_value("redeempath").get_str();
        std::string signature =  o.find_value("signature").get_str();

        if(signature.compare("") == 0) {
            continue;
        }
        if(getRedeemPathAvailable(allKeysArray,redeemPath)) {
            uint256 message = prepareMessageHash(messageIn);
            XOnlyPubKey xPubkey(CPubKey(ParseHex(redeemPath)));
            if(!xPubkey.VerifySchnorr(message,ParseHex(signature))) {
                LogPrintf("failed verfication \n");
            } else {
                thresold -= 1;
                if(thresold == 0) {
                    break;
                }
            }
        }
    }
    return thresold == 0 ? true : false;
}

bool validatePreconfSignature(std::string signatureHex, std::string messageIn, std::string prevWitnessHex) {

    std::vector<unsigned char> wData(ParseHex(prevWitnessHex));
    const std::string prevWitnessHexStr(wData.begin(), wData.end());
    UniValue witnessVal(UniValue::VOBJ);
    if (!witnessVal.read(prevWitnessHexStr)) {
        LogPrintf("invalid witness params \n");
        return false;
    }

    std::vector<std::string> allKeysArray;
    const auto allKeysArrayRequest = witnessVal.get_obj().find_value("all_keys").get_array();
    for (size_t i = 0; i < allKeysArrayRequest.size(); i++) {
        allKeysArray.push_back(allKeysArrayRequest[i].get_str());
    }

    int thresold = 1;
   
    std::vector<unsigned char> sData(ParseHex(signatureHex));
    const std::string signatureHexStr(sData.begin(), sData.end());
    UniValue allSignatures(UniValue::VARR);
    if (!allSignatures.read(signatureHexStr)) {
        LogPrintf("invalid signature params \n");
        return false;
    }
    for (unsigned int idx = 0; idx < allSignatures.size(); idx++) {
        const UniValue& o = allSignatures[idx].get_obj();
        RPCTypeCheckObj(o,
        {
            {"redeempath", UniValueType(UniValue::VSTR)},
            {"signature", UniValueType(UniValue::VSTR)},
        });
        std::string redeemPath =  o.find_value("redeempath").get_str();
        std::string signature =  o.find_value("signature").get_str();


        if(getRedeemPathAvailable(allKeysArray,redeemPath)) {
            uint256 message = prepareMessageHash(messageIn);

            XOnlyPubKey xPubkey(CPubKey(ParseHex(redeemPath)));
            if(!xPubkey.VerifySchnorr(message,ParseHex(signature))) {
                LogPrintf("failed verfication \n");
            } else {
                thresold -= 1;
                if(thresold == 0) {
                    break;
                }
            }
        }
    }
    return thresold == 0 ? true : false;
}

/**
 * Prepare sha256 hash for presigned block message
 */
uint256 prepareMessageHash(std::string message) {
    uint256 messageBuffer;
    CSHA256().Write((unsigned char*)message.data(), message.size()).Finalize(messageBuffer.begin());
    return messageBuffer;
}

/**
 * Signature path available in authorized anduro keys
 */
bool getRedeemPathAvailable(std::vector<std::string> fullQuorum, std::string signaturePath) {
    bool isSignaturePathExist = false;
    for (size_t i = 0; i < fullQuorum.size(); i++) {
        if(fullQuorum[i].compare(signaturePath) == 0) {
            isSignaturePathExist = true;
            break;
        }
    }

    return isSignaturePathExist;
}

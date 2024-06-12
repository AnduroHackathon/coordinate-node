#include <coordinate/coordinate_mempool_entry.h>
#include <anduro_validator.h>

std::vector<CoordinateMempoolEntry> coordinateMempoolEntry;

/**
 * This is the function which used to get mempool asset information
 */
bool getMempoolAsset(uint256 txid, uint32_t voutIn, CoordinateMempoolEntry* assetMempoolObj) {
    auto it = std::find_if(coordinateMempoolEntry.begin(), coordinateMempoolEntry.end(), 
                       [txid,voutIn] (const CoordinateMempoolEntry& d) { 
                          return d.txid == txid && d.vout == voutIn; 
                       });
    if (it == coordinateMempoolEntry.end()) return false;

    *assetMempoolObj = std::move(*it);
    return assetMempoolObj->assetID == 0 ? false : true;
}

/**
 * This is the function which remove all asset transaction based on txid
 */
void removeMempoolAsset(const CTransaction& tx) {
    uint256 txid = tx.GetHash();
    for (int32_t i = 0; i < tx.vout.size(); i++) {
        auto it = std::find_if(coordinateMempoolEntry.begin(), coordinateMempoolEntry.end(), 
                       [txid, i] (const CoordinateMempoolEntry& d) { 
                          return d.txid == txid && d.vout == i; ; 
                       });
        if (it == coordinateMempoolEntry.end()) {
        } else {
           int indexToRemove = it - coordinateMempoolEntry.begin() ;
           coordinateMempoolEntry.erase(coordinateMempoolEntry.begin() + indexToRemove);
        }
    }
}
/**
 * This is the function which include mempool asset
 */
void includeMempoolAsset(const CTransaction& tx, Chainstate& m_active_chainstate) {
    uint32_t currentAssetID = 0;
    CAmount amountAssetIn = 0;
    bool has_asset_amount = getAssetWithAmount(tx,m_active_chainstate,amountAssetIn, currentAssetID);
    if(has_asset_amount) {
        CAmount amountAssetOut = 0;
        for (int32_t i = 0; i < tx.vout.size(); i++) {
            if(amountAssetOut == amountAssetIn) {
                break;
            }
            CoordinateMempoolEntry assetMempoolObj;
            assetMempoolObj.assetID = currentAssetID;
            assetMempoolObj.txid = tx.GetHash();
            assetMempoolObj.vout = i;
            assetMempoolObj.nValue = tx.vout[i].nValue;
            coordinateMempoolEntry.push_back(assetMempoolObj);
            amountAssetOut = amountAssetOut + tx.vout[i].nValue;
        }
    }
}
/**
 * This is the function which used to get asset total amount
 */
bool getAssetWithAmount(const CTransaction& tx, Chainstate& m_active_chainstate, CAmount& amountAssetIn, uint32_t& currentAssetID) {
    CCoinsViewCache& mapInputs = m_active_chainstate.CoinsTip();
    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        uint32_t nAssetID = 0;
        bool fBitAsset = false;
        bool fBitAssetControl = false;

        CoordinateMempoolEntry assetMempoolObj;
        bool is_mempool_asset = getMempoolAsset(tx.vin[i].prevout.hash,tx.vin[i].prevout.n, &assetMempoolObj);
        nAssetID = assetMempoolObj.assetID;
        if(is_mempool_asset) {
            amountAssetIn = amountAssetIn + assetMempoolObj.nValue;
        } else {
            Coin coin;
            bool is_asset = mapInputs.getAssetCoin(tx.vin[i].prevout,fBitAsset,fBitAssetControl,nAssetID, &coin);
            if(fBitAssetControl) {
                currentAssetID = 0;
                break;
            }
            if(fBitAsset) {
                amountAssetIn = amountAssetIn + coin.out.nValue;
            }
        }


        if(!nAssetID) {
           break;
        } 
        currentAssetID = nAssetID;
    }

    return currentAssetID > 0 ? true : false;
}

/**
 * This is the function which get asset ouput information for particular transaction 
 */
int getAssetOutputCount(const CTransaction& tx, Chainstate& m_active_chainstate) {
    if(tx.nVersion == TRANSACTION_COORDINATE_ASSET_CREATE_VERSION) {
        return 2;
    }
    if(tx.nVersion == TRANSACTION_COORDINATE_ASSET_TRANSFER_VERSION) {
        uint32_t totalOutputs = 0;
        uint32_t currentAssetID = 0;
        CAmount amountAssetIn = 0;
        bool has_asset_amount = getAssetWithAmount(tx,m_active_chainstate,amountAssetIn, currentAssetID);
        if(has_asset_amount) {
            CAmount amountAssetOut = 0;
            for (unsigned int i = 0; i < tx.vout.size(); i++) {
                if(amountAssetOut == amountAssetIn) {
                    break;
                }
                totalOutputs = totalOutputs + 1;
                amountAssetOut = amountAssetOut + tx.vout[i].nValue;
            }
        }
        return totalOutputs;
    }

    return 0;
}
 
/**
 * This is the function which get mempool asset information through rpc
 */
std::vector<CoordinateMempoolEntry> getMempoolAssets() {
   return coordinateMempoolEntry;
}

#include <iostream>
#include <uint256.h>
#include <serialize.h>

template<typename Stream, typename CoordinateType>
inline void UnserializeAsset(CoordinateType& assetData, Stream& s) {
    s >> assetData.nID;
    s >> assetData.assetType;
    s >> assetData.precision;
    s >> assetData.strTicker;
    s >> assetData.strHeadline;
    s >> assetData.payload;
    s >> assetData.txid;
    s >> assetData.nSupply;
    s >> assetData.strController;
    s >> assetData.strOwner;
}

template<typename Stream, typename CoordinateType>
inline void SerializeAsset(const CoordinateType& assetData, Stream& s) {
    s << assetData.nID;
    s << assetData.assetType;
    s << assetData.precision;
    s << assetData.strTicker;
    s << assetData.strHeadline;
    s << assetData.payload;
    s << assetData.txid;
    s << assetData.nSupply;
    s << assetData.strController;
    s << assetData.strOwner;
}

struct CoordinateAsset {
public:
    uint32_t nID;   /*!< Asset unique number */
    uint32_t assetType; /*!< Asset type - (e.g. 0 - tokens, 1 - nft, 1 - blob nft) */
    uint32_t precision; /*!< Precision Number - (0..8) */
    std::string strTicker; /*!< Asset symbol. */
    std::string strHeadline; /*!< Asset name. */
    uint256 payload; /*!< Asset sha256 id combination for all asset information with asset data. */
    uint256 txid; /*!< Asset gensis id when the asset is newly minted */
    uint64_t nSupply; /*!< Asset current supply */
    std::string strController; /*!< Asset controller details, which used to mint additionally */
    std::string strOwner; /*!< Asset owner at the time of creating asset */

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        SerializeAsset(*this, s);
    }

    template <typename Stream>
    inline void Unserialize(Stream& s) {
        UnserializeAsset(*this, s);
    }

    template <typename Stream>
    CoordinateAsset(deserialize_type, Stream& s) {
        Unserialize(s);
    }

    CoordinateAsset() {
        SetNull();
    }

    void SetNull()
    {
        nID = 0;
        assetType = 0;
        precision = 0;
        strTicker="";
        strHeadline="";
        payload.SetNull();
        txid.SetNull();
        nSupply= 0;
        strController="";
        strOwner="";
    }
};


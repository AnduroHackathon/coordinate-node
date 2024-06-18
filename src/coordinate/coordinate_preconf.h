#include <iostream>
#include <uint256.h>
#include <serialize.h>
#include <validation.h>

template<typename Stream, typename CoordinatePreConfBlockType>
inline void UnserializeCoordinatePreConfBlock(CoordinatePreConfBlockType& blockData, Stream& s) {
    s >> blockData.fee;
    s >> blockData.txids;
    s >> blockData.pegins;
    s >> blockData.witness;
    s >> blockData.minedBlockHeight;
}

template<typename Stream, typename CoordinatePreConfBlockType>
inline void SerializeCoordinatePreConfBlock(const CoordinatePreConfBlockType& blockData, Stream& s) {
    s << blockData.fee;
    s << blockData.txids;
    s << blockData.pegins;
    s << blockData.witness;
    s << blockData.minedBlockHeight;
}

struct CoordinatePreConfBlock {
public:
    std::vector<uint256> txids; /*!< Preconf Mempool txid*/
    CAmount fee; /*!< final fee for signed block*/
    std::vector<CTxOut> pegins; /*!< pegin information */
    std::string witness;
    int64_t minedBlockHeight; /*!< block height to verify witness */

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        SerializeCoordinatePreConfBlock(*this, s);
    }

    template <typename Stream>
    inline void Unserialize(Stream& s) {
        UnserializeCoordinatePreConfBlock(*this, s);
    }

    template <typename Stream>
    CoordinatePreConfBlock(deserialize_type, Stream& s) {
        Unserialize(s);
    }

    CoordinatePreConfBlock() {
        SetNull();
    }

    void SetNull()
    {
        fee = -1;
        static_cast<void>(txids.empty());
        static_cast<void>(pegins.empty());
        witness = "";
        minedBlockHeight = -1;
    }
};




template<typename Stream, typename CoordinatePreConfSigType>
inline void UnserializeCoordinatePreConfSig(CoordinatePreConfSigType& preconfData, Stream& s) {
    s >> preconfData.txid;
    s >> preconfData.blockHeight;
    s >> preconfData.minedBlockHeight;
    s >> preconfData.reserve;
    s >> preconfData.vsize;
    s >> preconfData.witness;
    s >> preconfData.isBroadcasted;
    s >> preconfData.peerList;
    s >> preconfData.federationKey;
    s >> preconfData.finalized;
    s >> preconfData.pegins;
}

template<typename Stream, typename CoordinatePreConfSigType>
inline void SerializeCoordinatePreConfSig(const CoordinatePreConfSigType& preconfData, Stream& s) {
    s << preconfData.txid;
    s << preconfData.blockHeight;
    s << preconfData.minedBlockHeight;
    s << preconfData.reserve;
    s << preconfData.vsize;
    s << preconfData.witness;
    s << preconfData.isBroadcasted;
    s << preconfData.peerList;
    s << preconfData.federationKey;
    s << preconfData.finalized;
    s << preconfData.pegins;
}

struct CoordinatePreConfSig {
public:
    uint256 txid; /*!< Preconf Mempool txid*/
    int64_t blockHeight; /*!< max block height where the preconf signature upto valid */
    int64_t minedBlockHeight; /*!< federation public key reference in mined block height*/
    int32_t reserve; /*!< transaction reserve */
    int32_t vsize; /*!< transaction virtula size */
    std::string witness;
    std::string federationKey; /*!< federation public key */
    uint32_t finalized; /*!< 0 - not finalized by federation, 1 - finalized by federation */
    bool isBroadcasted; /*!< identify that it was broadcasted to the peers */
    std::vector<int64_t> peerList;  /*!< connected peer array that federation signed list sent through network*/
    std::vector<CTxOut> pegins;  /*!< pegin list from anduro federation*/

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        SerializeCoordinatePreConfSig(*this, s);
    }

    template <typename Stream>
    inline void Unserialize(Stream& s) {
        UnserializeCoordinatePreConfSig(*this, s);
    }

    template <typename Stream>
    CoordinatePreConfSig(deserialize_type, Stream& s) {
        Unserialize(s);
    }

    CoordinatePreConfSig() {
        SetNull();
    }

    void SetNull()
    {
        blockHeight = -1;
        reserve = -1;
        vsize = -1;
        minedBlockHeight = -1;
        txid.SetNull();
        witness = "";
        isBroadcasted = false;
        static_cast<void>(peerList.empty());
        federationKey = "";
        static_cast<void>(pegins.empty());
    }
};


/**
 * This function get mempool entry based on preconf signature
 */
CoordinatePreConfBlock getNextPreConfSigList(ChainstateManager& chainman);

/**
 * This function include preconf witness from anduro
 * @param[in] preconf hold three block presigned signature.
 * @param[in] chainman  used to find previous blocks based on active chain state to valid preconf signatures
 */
bool includePreConfSigWitness(std::vector<CoordinatePreConfSig> preconf, ChainstateManager& chainman);

/**
 * This function include preconf witness from anduro
 * @param[in] newFinalizedSignedBlocks hold signed block list from network
 * @param[in] chainman  used to find previous blocks based on active chain state to valid preconf signatures
 */
bool includePreConfBlockFromNetwork(std::vector<SignedBlock> newFinalizedSignedBlocks, ChainstateManager& chainman);

/**
 * This function used to remove preconf signature afer included to block
 * @param[in] chainman  used to find previous blocks based on active chain state to valid preconf signatures
 */
void removePreConfSigWitness(ChainstateManager& chainman);

/**
 * This is the function which used to get unbroadcasted preconfirmation signatures
 */
std::vector<CoordinatePreConfSig> getUnBroadcastedPreConfSig();

/**
 * This is the function which used to get unbroadcasted preconfirmation signed block
 */
std::vector<SignedBlock> getUnBroadcastedPreConfSignedBlock();

/**
 * This is the function which used change status for broadcasted preconf
 * @param[in] preconfItem current preconf list send to network
 * @param[in] peerId  current preconf received peer id
 */
void updateBroadcastedPreConf(CoordinatePreConfSig& preconfItem, int64_t peerId);


/**
 * This is the function which used change status for broadcasted signed block
 * @param[in] signedBlockItem current signed block list send to network
 * @param[in] peerId  current preconf received peer id
 */
void updateBroadcastedSignedBlock(SignedBlock& signedBlockItem, int64_t peerId);



/**
 * Calculate next block preconf fee based on selected list
 * @param[in] preconf_pool preconf pool current object from active chain state
 * @param[in] signedBlockHeight Signed block height to calculate fee
*/
CAmount nextBlockFee(CTxMemPool& preconf_pool, uint64_t signedBlockHeight);

/**
 * Calculate next block refunds
 * @param[in] preconf_pool preconf pool current object from active chain state
 * @param[in] finalFee final fee for current signed block
 * @param[in] signedBlockHeight Signed block height to calculate refunds
*/
CoordinatePreConfBlock prepareRefunds(CTxMemPool& preconf_pool, CAmount finalFee, uint64_t signedBlockHeight);
/**
 * This is the function which used to get all preconfirmation signatures list
 */
std::vector<CoordinatePreConfSig> getPreConfSig();

/**
 * This function remove old federation signature for memmory
 */
void removePreConfWitness();

/**
 * This function remove finalized preconf block after included in mined block
 * @param[in] blockHeight block height to expire the preconf finalized block list
 */
void removePreConfFinalizedBlock(uint64_t blockHeight);

/**
 * This function return preconf minimum fee
 */
CAmount getPreConfMinFee();

/**
 * This function get new block template for signed block
 * @param[in] chainman  used to find previous blocks based on active chain state
 * @param[in] nTime utc time to include the signed block time
 */
std::unique_ptr<SignedBlock> CreateNewSignedBlock(ChainstateManager& chainman, uint32_t nTime);

/**
 * This function check whether signed block is valid or not based on federation witness
 * @param[in] block signed block details
 * @param[in] chainman  used to find previous blocks based on active chain state
 */
bool checkSignedBlock(const SignedBlock& block, ChainstateManager& chainman);


/**
 * This function will get all invalid mined tx details for bloks
 * @param[in] chainman  used to find previous blocks based on active chain state
 */
std::vector<uint256> getInvalidTx(ChainstateManager& chainman);

/**
 * This function will get reconsile mined block hash
 * @param[in] chainman  used to find previous blocks based on active chain state
 */
uint256 getReconsiledBlock(ChainstateManager& chainman);

/**
 * This function will get preconf fee for block based on signed block height
 * @param[in] chainman  used to find previous blocks based on active chain state
 * @param[in] blockHeight signed block height
 */
CAmount getPreconfFeeForBlock(ChainstateManager& chainman, int blockHeight);

/**
 * This function will get mined block fee based on block height
 * @param[in] chainman  used to find previous blocks based on active chain state
 * @param[in] blockHeight mined block height
 */
CAmount getFeeForBlock(ChainstateManager& chainman, int blockHeight);

/**
 * This function will get miner details who solved the block using auxpow
 * @param[in] chainman  used to find previous blocks based on active chain state
 * @param[in] blockHeight mined block height
 */
CScript getMinerScript(ChainstateManager& chainman, int blockHeight);

/**
 * This function will get curren federation script key for preconf reward
 * @param[in] chainman  used to find previous blocks based on active chain state
 * @param[in] blockHeight mined block height
 */
CScript getFederationScript(ChainstateManager& chainman, int blockHeight);

/**
 * This is the function which used to get all finalized signed block
 */
std::vector<SignedBlock> getFinalizedSignedBlocks();

/**
 * This function will insert new signed block in memory
 * @param[in] newFinalizedSignedBlock  signed block detail
 */
void insertNewSignedBlock(const SignedBlock& newFinalizedSignedBlock);

/**
 * This function will calculate federation fee for particular signed block
 * @param[in] vtx signed block transactioh list
 * @param[in] currentFee current signed block fee
 */
CAmount getPreconfFeeForFederation(std::vector<CTransactionRef> vtx, CAmount currentFee);

/**
 * This function will calculate refund for particular tx in signed block
 * @param[in] ptx  used to find previous blocks based on active chain state
 * @param[in] blockFee signed block current fee
 * @param[in] inputs active coin tip
 */
CAmount getRefundForPreconfTx(const CTransaction& ptx, CAmount blockFee, CCoinsViewCache& inputs) ;


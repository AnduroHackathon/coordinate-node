#ifndef BITCOIN_SIGNEDTXINDEX_H
#define BITCOIN_SIGNEDTXINDEX_H

#include <serialize.h>
#include <uint256.h>

class SignedTxindex {
    public:
        uint32_t pos; /*!< signed tx position where it positioned in signed block */
        int blockIndex; /*!< mined block index where signed block confirmed */
        uint256 signedBlockHash; /*!< signed block hash where tx included */

        SignedTxindex() {
         SetNull();
        }

        SERIALIZE_METHODS(SignedTxindex, obj) { READWRITE(obj.pos, obj.blockIndex, obj.signedBlockHash);}
        void SetNull()
        {
            pos = 0;
            blockIndex = 0;
            signedBlockHash.SetNull();
        }
};

#endif // BITCOIN_SIGNEDTXINDEX_H
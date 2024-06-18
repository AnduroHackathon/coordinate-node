#ifndef BITCOIN_INVALIDTX_H
#define BITCOIN_INVALIDTX_H

#include <serialize.h>
#include <uint256.h>

class InvalidTx {
    public:
        std ::vector<uint256> invalidTxs; /*!< all invalid tx hash for mined block */
        uint64_t nHeight; /*!< mined block height */
        InvalidTx() {
         SetNull();
        }

        SERIALIZE_METHODS(InvalidTx, obj) { READWRITE(obj.invalidTxs, obj.nHeight);}

        void SetNull()
        {
            static_cast<void>(invalidTxs.empty());
            nHeight = 0;
        }
};

#endif // BITCOIN_INVALIDTX_H

// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txdb.h>

#include <coins.h>
#include <dbwrapper.h>
#include <logging.h>
#include <primitives/transaction.h>
#include <random.h>
#include <serialize.h>
#include <uint256.h>
#include <util/vector.h>

#include <cassert>
#include <cstdlib>
#include <iterator>
#include <utility>

static constexpr uint8_t DB_COIN{'C'};
static constexpr uint8_t DB_BEST_BLOCK{'B'};
static constexpr uint8_t DB_HEAD_BLOCKS{'H'};
// Keys used in previous version that might still be found in the DB:
static constexpr uint8_t DB_COINS{'c'};

static constexpr uint8_t DB_ASSET{'A'};
static constexpr uint8_t DB_MINED_ASSET{'D'};
static constexpr uint8_t DB_ASSET_LAST_ID{'I'};
static constexpr uint8_t DB_ASSET_LAST_PRUNE_HEIGHT{'E'};

static constexpr uint8_t DB_SIGNED_BLOCK_HASH{'V'};
static constexpr uint8_t DB_SIGNED_BLOCK_LAST_ID{'S'};
static constexpr uint8_t DB_SIGNED_BLOCK_LAST_HASH{'U'};
static constexpr uint8_t DB_BLOCK_INVALID_TX{'Q'};
static constexpr uint8_t DB_SIGNED_BLOCK_TX{'P'};


bool CCoinsViewDB::NeedsUpgrade()
{
    std::unique_ptr<CDBIterator> cursor{m_db->NewIterator()};
    // DB_COINS was deprecated in v0.15.0, commit
    // 1088b02f0ccd7358d2b7076bb9e122d59d502d02
    cursor->Seek(std::make_pair(DB_COINS, uint256{}));
    return cursor->Valid();
}

namespace {

struct CoinEntry {
    COutPoint* outpoint;
    uint8_t key;
    explicit CoinEntry(const COutPoint* ptr) : outpoint(const_cast<COutPoint*>(ptr)), key(DB_COIN)  {}

    SERIALIZE_METHODS(CoinEntry, obj) { READWRITE(obj.key, obj.outpoint->hash, VARINT(obj.outpoint->n)); }
};

} // namespace

CCoinsViewDB::CCoinsViewDB(DBParams db_params, CoinsViewOptions options) :
    m_db_params{std::move(db_params)},
    m_options{std::move(options)},
    m_db{std::make_unique<CDBWrapper>(m_db_params)} { }

void CCoinsViewDB::ResizeCache(size_t new_cache_size)
{
    // We can't do this operation with an in-memory DB since we'll lose all the coins upon
    // reset.
    if (!m_db_params.memory_only) {
        // Have to do a reset first to get the original `m_db` state to release its
        // filesystem lock.
        m_db.reset();
        m_db_params.cache_bytes = new_cache_size;
        m_db_params.wipe_data = false;
        m_db = std::make_unique<CDBWrapper>(m_db_params);
    }
}

bool CCoinsViewDB::GetCoin(const COutPoint &outpoint, Coin &coin) const {
    return m_db->Read(CoinEntry(&outpoint), coin);
}

bool CCoinsViewDB::HaveCoin(const COutPoint &outpoint) const {
    return m_db->Exists(CoinEntry(&outpoint));
}

uint256 CCoinsViewDB::GetBestBlock() const {
    uint256 hashBestChain;
    if (!m_db->Read(DB_BEST_BLOCK, hashBestChain))
        return uint256();
    return hashBestChain;
}

std::vector<uint256> CCoinsViewDB::GetHeadBlocks() const {
    std::vector<uint256> vhashHeadBlocks;
    if (!m_db->Read(DB_HEAD_BLOCKS, vhashHeadBlocks)) {
        return std::vector<uint256>();
    }
    return vhashHeadBlocks;
}

bool CCoinsViewDB::BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlock, bool erase) {
    CDBBatch batch(*m_db);
    size_t count = 0;
    size_t changed = 0;
    assert(!hashBlock.IsNull());

    uint256 old_tip = GetBestBlock();
    if (old_tip.IsNull()) {
        // We may be in the middle of replaying.
        std::vector<uint256> old_heads = GetHeadBlocks();
        if (old_heads.size() == 2) {
            if (old_heads[0] != hashBlock) {
                LogPrintLevel(BCLog::COINDB, BCLog::Level::Error, "The coins database detected an inconsistent state, likely due to a previous crash or shutdown. You will need to restart coordinated with the -reindex-chainstate or -reindex configuration option.\n");
            }
            assert(old_heads[0] == hashBlock);
            old_tip = old_heads[1];
        }
    }

    // In the first batch, mark the database as being in the middle of a
    // transition from old_tip to hashBlock.
    // A vector is used for future extensibility, as we may want to support
    // interrupting after partial writes from multiple independent reorgs.
    batch.Erase(DB_BEST_BLOCK);
    batch.Write(DB_HEAD_BLOCKS, Vector(hashBlock, old_tip));

    for (CCoinsMap::iterator it = mapCoins.begin(); it != mapCoins.end();) {
        if (it->second.flags & CCoinsCacheEntry::DIRTY) {
            CoinEntry entry(&it->first);
            if (it->second.coin.IsSpent())
                batch.Erase(entry);
            else
                batch.Write(entry, it->second.coin);
            changed++;
        }
        count++;
        it = erase ? mapCoins.erase(it) : std::next(it);
        if (batch.SizeEstimate() > m_options.batch_write_bytes) {
            LogPrint(BCLog::COINDB, "Writing partial batch of %.2f MiB\n", batch.SizeEstimate() * (1.0 / 1048576.0));
            m_db->WriteBatch(batch);
            batch.Clear();
            if (m_options.simulate_crash_ratio) {
                static FastRandomContext rng;
                if (rng.randrange(m_options.simulate_crash_ratio) == 0) {
                    LogPrintf("Simulating a crash. Goodbye.\n");
                    _Exit(0);
                }
            }
        }
    }

    // In the last batch, mark the database as consistent with hashBlock again.
    batch.Erase(DB_HEAD_BLOCKS);
    batch.Write(DB_BEST_BLOCK, hashBlock);

    LogPrint(BCLog::COINDB, "Writing final batch of %.2f MiB\n", batch.SizeEstimate() * (1.0 / 1048576.0));
    bool ret = m_db->WriteBatch(batch);
    LogPrint(BCLog::COINDB, "Committed %u changed transaction outputs (out of %u) to coin database...\n", (unsigned int)changed, (unsigned int)count);
    return ret;
}

size_t CCoinsViewDB::EstimateSize() const
{
    return m_db->EstimateSize(DB_COIN, uint8_t(DB_COIN + 1));
}

/** Specialization of CCoinsViewCursor to iterate over a CCoinsViewDB */
class CCoinsViewDBCursor: public CCoinsViewCursor
{
public:
    // Prefer using CCoinsViewDB::Cursor() since we want to perform some
    // cache warmup on instantiation.
    CCoinsViewDBCursor(CDBIterator* pcursorIn, const uint256&hashBlockIn):
        CCoinsViewCursor(hashBlockIn), pcursor(pcursorIn) {}
    ~CCoinsViewDBCursor() = default;

    bool GetKey(COutPoint &key) const override;
    bool GetValue(Coin &coin) const override;

    bool Valid() const override;
    void Next() override;

private:
    std::unique_ptr<CDBIterator> pcursor;
    std::pair<char, COutPoint> keyTmp;

    friend class CCoinsViewDB;
};

std::unique_ptr<CCoinsViewCursor> CCoinsViewDB::Cursor() const
{
    auto i = std::make_unique<CCoinsViewDBCursor>(
        const_cast<CDBWrapper&>(*m_db).NewIterator(), GetBestBlock());
    /* It seems that there are no "const iterators" for LevelDB.  Since we
       only need read operations on it, use a const-cast to get around
       that restriction.  */
    i->pcursor->Seek(DB_COIN);
    // Cache key of first record
    if (i->pcursor->Valid()) {
        CoinEntry entry(&i->keyTmp.second);
        i->pcursor->GetKey(entry);
        i->keyTmp.first = entry.key;
    } else {
        i->keyTmp.first = 0; // Make sure Valid() and GetKey() return false
    }
    return i;
}

bool CCoinsViewDBCursor::GetKey(COutPoint &key) const
{
    // Return cached key
    if (keyTmp.first == DB_COIN) {
        key = keyTmp.second;
        return true;
    }
    return false;
}

bool CCoinsViewDBCursor::GetValue(Coin &coin) const
{
    return pcursor->GetValue(coin);
}

bool CCoinsViewDBCursor::Valid() const
{
    return keyTmp.first == DB_COIN;
}

void CCoinsViewDBCursor::Next()
{
    pcursor->Next();
    CoinEntry entry(&keyTmp.second);
    if (!pcursor->Valid() || !pcursor->GetKey(entry)) {
        keyTmp.first = 0; // Invalidate cached key after last record so that Valid() and GetKey() return false
    } else {
        keyTmp.first = entry.key;
    }
}

CoordinateAssetDB::CoordinateAssetDB(DBParams db_params)
    : CDBWrapper(db_params) { }

bool CoordinateAssetDB::WriteCoordinateAssets(const std::vector<CoordinateAsset>& vAsset)
{
    CDBBatch batch(*this);
    for (const CoordinateAsset& asset : vAsset) {
        std::pair<uint8_t, uint32_t> key = std::make_pair(DB_ASSET, asset.nID);
        batch.Write(key, asset);
    }
    return WriteBatch(batch, true);
}

std::vector<CoordinateAsset> CoordinateAssetDB::GetAssets()
{
    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->Seek(std::make_pair(DB_ASSET, uint256()));

    std::vector<CoordinateAsset> vAsset;

    while (pcursor->Valid()) {
        std::pair<uint8_t, uint32_t> key;
        CoordinateAsset asset;
        if (pcursor->GetKey(key) && key.first == DB_ASSET) {
            if (pcursor->GetValue(asset))
                vAsset.push_back(asset);
        }

        pcursor->Next();
    }
    return vAsset;
}

bool CoordinateAssetDB::GetLastAssetID(uint32_t& nID)
{
    // Look up the last asset ID (in chronological order)
    if (!Read(DB_ASSET_LAST_ID, nID))
        return false;

    return true;
}

bool CoordinateAssetDB::WriteLastAssetID(const uint32_t nID)
{
    return Write(DB_ASSET_LAST_ID, nID);
}

bool CoordinateAssetDB::GetLastAssetPruneHeight(uint32_t& nID)
{
    if (!Read(DB_ASSET_LAST_PRUNE_HEIGHT, nID))
        return false;

    return true;
}

bool CoordinateAssetDB::WriteLastAssetPruneHeight(const uint32_t nID)
{
    return Write(DB_ASSET_LAST_PRUNE_HEIGHT, nID);
}


bool CoordinateAssetDB::RemoveAsset(const uint32_t nID)
{
    std::pair<uint8_t, uint32_t> key = std::make_pair(DB_ASSET, nID);
    return Erase(key);
}

bool CoordinateAssetDB::GetAsset(const uint32_t nID, CoordinateAsset& asset)
{
    return Read(std::make_pair(DB_ASSET, nID), asset);
}

bool CoordinateAssetDB::WriteAssetMinedBlock(uint256 blockHash) {
    CDBBatch batch(*this);
    std::pair<uint8_t, uint256> key = std::make_pair(DB_MINED_ASSET, blockHash);
    batch.Write(key, blockHash);
    return WriteBatch(batch, true);
}

bool CoordinateAssetDB::getAssetMinedBlock(uint256 blockHash) {
    uint256 hash;
    if(Read(std::make_pair(DB_MINED_ASSET, blockHash), hash)) {
        return true;
    }
    return false;
}

SignedBlocksDB::SignedBlocksDB(DBParams db_params)
    : CDBWrapper(db_params) { }

bool SignedBlocksDB::WriteLastSignedBlockID(const uint64_t nHeight)
{
    return Write(DB_SIGNED_BLOCK_LAST_ID, nHeight);
}

bool SignedBlocksDB::GetLastSignedBlockID(uint64_t& nHeight)
{
    if (!Read(DB_SIGNED_BLOCK_LAST_ID, nHeight))
        return false;

    return true;
}

bool SignedBlocksDB::WriteLastSignedBlockHash(const uint256 blockHash)
{
    return Write(DB_SIGNED_BLOCK_LAST_HASH, blockHash);
}

bool SignedBlocksDB::GetLastSignedBlockHash(uint256& blockHash)
{
    if (!Read(DB_SIGNED_BLOCK_LAST_HASH, blockHash))
        return false;

    return true;
}

bool SignedBlocksDB::WriteSignedBlockHash(const std::vector<uint256>& signedBlockHashes, uint256 blockHash)
{
    CDBBatch batch(*this);
    for (const uint256& signedBlockHash : signedBlockHashes) {
        std::pair<uint8_t, uint256> key = std::make_pair(DB_SIGNED_BLOCK_HASH, signedBlockHash);
        batch.Write(key, blockHash);
    }
    return WriteBatch(batch, true);
}

bool SignedBlocksDB::GetSignedBlockHash(const uint256 signedBlockHashes, uint256& blockHash)
{
    return Read(std::make_pair(DB_SIGNED_BLOCK_HASH, signedBlockHashes), blockHash);
}

bool SignedBlocksDB::WriteInvalidTx(const std::vector<InvalidTx>& invalidTxs){
    CDBBatch batch(*this);
    for (const InvalidTx& invalidTx : invalidTxs) {
        std::pair<uint8_t, uint64_t> key = std::make_pair(DB_BLOCK_INVALID_TX, invalidTx.nHeight);
        batch.Write(key, invalidTx);
    }
    return WriteBatch(batch, true);
}

bool SignedBlocksDB::GetInvalidTx(const uint64_t nHeight, InvalidTx& invalidTx)
{
    return Read(std::make_pair(DB_BLOCK_INVALID_TX, nHeight), invalidTx);
}


bool SignedBlocksDB::WriteTxPosition(const SignedTxindex& signedTx, uint256 txHash){
    CDBBatch batch(*this);
    std::pair<uint8_t, uint256> key = std::make_pair(DB_SIGNED_BLOCK_TX, txHash);
    batch.Write(key, signedTx);
    return WriteBatch(batch, true);
}

bool SignedBlocksDB::getTxPosition(const uint256 txHash, SignedTxindex& txIndex)
{
    return Read(std::make_pair(DB_SIGNED_BLOCK_TX, txHash), txIndex);
}
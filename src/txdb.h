// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COORDINATE_TXDB_H
#define COORDINATE_TXDB_H

#include <coins.h>
#include <dbwrapper.h>
#include <kernel/cs_main.h>
#include <sync.h>
#include <util/fs.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>
#include <coordinate/coordinate_assets.h>
#include <coordinate/signed_block.h>
#include <coordinate/invalid_tx.h>
#include <coordinate/signed_txindex.h>

class COutPoint;
class uint256;

//! -dbcache default (MiB)
static const int64_t nDefaultDbCache = 450;
//! -dbbatchsize default (bytes)
static const int64_t nDefaultDbBatchSize = 16 << 20;
//! max. -dbcache (MiB)
static const int64_t nMaxDbCache = sizeof(void*) > 4 ? 16384 : 1024;
//! min. -dbcache (MiB)
static const int64_t nMinDbCache = 4;
//! Max memory allocated to block tree DB specific cache, if no -txindex (MiB)
static const int64_t nMaxBlockDBCache = 2;
//! Max memory allocated to block tree DB specific cache, if -txindex (MiB)
// Unlike for the UTXO database, for the txindex scenario the leveldb cache make
// a meaningful difference: https://github.com/bitcoin/bitcoin/pull/8273#issuecomment-229601991
static const int64_t nMaxTxIndexCache = 1024;
//! Max memory allocated to all block filter index caches combined in MiB.
static const int64_t max_filter_index_cache = 1024;
//! Max memory allocated to coin DB specific cache (MiB)
static const int64_t nMaxCoinsDBCache = 8;

//! User-controlled performance and debug options.
struct CoinsViewOptions {
    //! Maximum database write batch size in bytes.
    size_t batch_write_bytes = nDefaultDbBatchSize;
    //! If non-zero, randomly exit when the database is flushed with (1/ratio)
    //! probability.
    int simulate_crash_ratio = 0;
};

/** CCoinsView backed by the coin database (chainstate/) */
class CCoinsViewDB final : public CCoinsView
{
protected:
    DBParams m_db_params;
    CoinsViewOptions m_options;
    std::unique_ptr<CDBWrapper> m_db;
public:
    explicit CCoinsViewDB(DBParams db_params, CoinsViewOptions options);

    bool GetCoin(const COutPoint &outpoint, Coin &coin) const override;
    bool HaveCoin(const COutPoint &outpoint) const override;
    uint256 GetBestBlock() const override;
    std::vector<uint256> GetHeadBlocks() const override;
    bool BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlock, bool erase = true) override;
    std::unique_ptr<CCoinsViewCursor> Cursor() const override;

    //! Whether an unsupported database format is used.
    bool NeedsUpgrade();
    size_t EstimateSize() const override;

    //! Dynamically alter the underlying leveldb cache size.
    void ResizeCache(size_t new_cache_size) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    //! @returns filesystem path to on-disk storage or std::nullopt if in memory.
    std::optional<fs::path> StoragePath() { return m_db->StoragePath(); }
};

/** Access to the CoordinateAsset database (blocks/CoordinateAssets/) */
class CoordinateAssetDB : public CDBWrapper
{
public:
    CoordinateAssetDB(DBParams db_params);
    bool WriteCoordinateAssets(const std::vector<CoordinateAsset>& vAsset);
    std::vector<CoordinateAsset> GetAssets();
    bool GetLastAssetID(uint32_t& nID);
    bool WriteLastAssetID(const uint32_t nID);
    bool GetAsset(const uint32_t nID,CoordinateAsset& asset);
    bool RemoveAsset(const uint32_t nID);
    bool WriteAssetMinedBlock(uint256 blockHash);
    bool getAssetMinedBlock(uint256 blockHash);
    bool GetLastAssetPruneHeight(uint32_t& nID);
    bool WriteLastAssetPruneHeight(const uint32_t nID);
};

/** Access to the signed blocks database (blocks/signedblocks/) */
class SignedBlocksDB : public CDBWrapper {
public:
    SignedBlocksDB(DBParams db_params);
    bool GetLastSignedBlockID(uint64_t& nHeight);
    bool WriteLastSignedBlockID(const uint64_t nHeight);
    bool GetLastSignedBlockHash(uint256& blockHash);
    bool WriteLastSignedBlockHash(const uint256 blockHash);
    bool WriteSignedBlockHash(const std::vector<uint256>& signedBlockHashes, uint256 blockHash);
    bool GetSignedBlockHash(const uint256 signedBlockHashes, uint256& blockHash);
    bool WriteInvalidTx(const std::vector<InvalidTx>& invalidTxs);
    bool GetInvalidTx(const uint64_t nHeight, InvalidTx& invalidTx);
    bool WriteTxPosition(const SignedTxindex& signedTx, uint256 txHash);
    bool getTxPosition(const uint256 txHash, SignedTxindex& txIndex);
};



#endif // COORDINATE_TXDB_H

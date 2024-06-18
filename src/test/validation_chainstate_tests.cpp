// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <chainparams.h>
#include <consensus/validation.h>
#include <random.h>
#include <rpc/blockchain.h>
#include <sync.h>
#include <test/util/chainstate.h>
#include <test/util/coins.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <validation.h>

#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(validation_chainstate_tests, ChainTestingSetup)

//! Test resizing coins-related Chainstate caches during runtime.
//!
BOOST_AUTO_TEST_CASE(validation_chainstate_resize_caches)
{
    ChainstateManager& manager = *Assert(m_node.chainman);
    CTxMemPool& mempool = *Assert(m_node.mempool);
    CTxMemPool& preconfmempool = *Assert(m_node.preconfmempool);
    Chainstate& c1 = WITH_LOCK(cs_main, return manager.InitializeChainstate(&mempool, &preconfmempool));
    c1.InitCoinsDB(
        /*cache_size_bytes=*/1 << 23, /*in_memory=*/true, /*should_wipe=*/false);
    WITH_LOCK(::cs_main, c1.InitCoinsCache(1 << 23));
    BOOST_REQUIRE(c1.LoadGenesisBlock()); // Need at least one block loaded to be able to flush caches

    // Add a coin to the in-memory cache, upsize once, then downsize.
    {
        LOCK(::cs_main);
        const auto outpoint = AddTestCoin(c1.CoinsTip());

        // Set a meaningless bestblock value in the coinsview cache - otherwise we won't
        // flush during ResizecoinsCaches() and will subsequently hit an assertion.
        c1.CoinsTip().SetBestBlock(InsecureRand256());

        BOOST_CHECK(c1.CoinsTip().HaveCoinInCache(outpoint));

        c1.ResizeCoinsCaches(
            1 << 24,  // upsizing the coinsview cache
            1 << 22  // downsizing the coinsdb cache
        );

        // View should still have the coin cached, since we haven't destructed the cache on upsize.
        BOOST_CHECK(c1.CoinsTip().HaveCoinInCache(outpoint));

        c1.ResizeCoinsCaches(
            1 << 22,  // downsizing the coinsview cache
            1 << 23  // upsizing the coinsdb cache
        );

        // The view cache should be empty since we had to destruct to downsize.
        BOOST_CHECK(!c1.CoinsTip().HaveCoinInCache(outpoint));
    }
}


BOOST_AUTO_TEST_SUITE_END()

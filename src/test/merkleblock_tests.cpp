// Copyright (c) 2012-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <merkleblock.h>
#include <consensus/merkle.h>
#include <test/util/setup_common.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

#include <set>
#include <vector>

BOOST_AUTO_TEST_SUITE(merkleblock_tests)

/**
 * Create a CMerkleBlock using a list of txids which will be found in the
 * given block.
 */
BOOST_FIXTURE_TEST_CASE(merkleblock_construct_from_txids_found, TestChain100Setup)
{
    CBlock block = getBlock13b8a();

    std::set<uint256> txids;
    // Create transaction 1
    CMutableTransaction coinbaseTx = CMutableTransaction();
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    coinbaseTx.nVersion = 1;
    coinbaseTx.vin.resize(1);
    coinbaseTx.vout.resize(1);
    coinbaseTx.vin[0].scriptSig = CScript() << OP_11 << OP_EQUAL;
    coinbaseTx.vout[0].nValue = 1 * CENT;
    const uint256& hashv = coinbaseTx.GetHash();
    txids.insert(hashv);
    block.vtx.push_back (MakeTransactionRef (coinbaseTx));
      // Create transaction 2
    CMutableTransaction coinbaseTx2 = CMutableTransaction();
    coinbaseTx2.nVersion = 1;
    coinbaseTx2.vin.resize(1);
    coinbaseTx2.vout.resize(1);
    coinbaseTx2.vin[0].scriptSig = CScript() << OP_11 << OP_EQUAL;
    coinbaseTx2.vout[0].nValue = 2 * CENT;
    const uint256& hashv2 = coinbaseTx2.GetHash();
    txids.insert(hashv2);
    block.vtx.push_back (MakeTransactionRef (coinbaseTx2));

    std::vector<uint256> vMatched;
    std::vector<unsigned int> vIndex;
    CMerkleBlock merkleBlock(block, txids);
    BOOST_CHECK_EQUAL(merkleBlock.header.GetHash().GetHex(), block.GetHash().GetHex());

    // vMatchedTxn is only used when bloom filter is specified.
    BOOST_CHECK_EQUAL(merkleBlock.vMatchedTxn.size(), 0U);
    // Calculate hashmerkle root
    block.hashMerkleRoot = merkleBlock.txn.ExtractMatches(vMatched, vIndex);

    BOOST_CHECK_EQUAL(merkleBlock.txn.ExtractMatches(vMatched, vIndex).GetHex(), block.hashMerkleRoot.GetHex());
    BOOST_CHECK_EQUAL(vMatched.size(), 2U);

    // Ordered by occurrence in depth-first tree traversal.
    BOOST_CHECK_EQUAL(vMatched[0].ToString(), hashv.ToString());
    BOOST_CHECK_EQUAL(vIndex[0], 0U);

    BOOST_CHECK_EQUAL(vMatched[1].ToString(), hashv2.ToString());
    BOOST_CHECK_EQUAL(vIndex[1], 1U);
}


/**
 * Create a CMerkleBlock using a list of txids which will not be found in the
 * given block.
 */
BOOST_FIXTURE_TEST_CASE(merkleblock_construct_from_txids_not_found, TestChain100Setup)
{
    CBlock block = getBlock13b8a();

    std::set<uint256> txids2;
    txids2.insert(uint256S("0xc0ffee00003bafa802c8aa084379aa98d9fcd632ddc2ed9782b586ec87451f20"));
    // Create transaction 1
    CMutableTransaction coinbaseTx = CMutableTransaction();
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    coinbaseTx.nVersion = 1;
    coinbaseTx.vin.resize(1);
    coinbaseTx.vout.resize(1);
    coinbaseTx.vin[0].scriptSig = CScript() << OP_11 << OP_EQUAL;
    coinbaseTx.vout[0].nValue = 3 * CENT;
    const uint256& hashv = coinbaseTx.GetHash();
    block.vtx.push_back (MakeTransactionRef (coinbaseTx));

    CMerkleBlock merkleBlock(block, txids2);

    BOOST_CHECK_EQUAL(merkleBlock.header.GetHash().GetHex(), block.GetHash().GetHex());
    BOOST_CHECK_EQUAL(merkleBlock.vMatchedTxn.size(), 0U);

    std::vector<uint256> vMatched;
    std::vector<unsigned int> vIndex;
    block.hashMerkleRoot = merkleBlock.txn.ExtractMatches(vMatched, vIndex);

    BOOST_CHECK_EQUAL(merkleBlock.txn.ExtractMatches(vMatched, vIndex).GetHex(), block.hashMerkleRoot.GetHex());
    BOOST_CHECK_EQUAL(vMatched.size(), 0U);
    BOOST_CHECK_EQUAL(vIndex.size(), 0U);
}

BOOST_AUTO_TEST_SUITE_END()

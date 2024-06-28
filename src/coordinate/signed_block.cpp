
#include <coordinate/signed_block.h>
#include <hash.h>

uint256 SignedBlock::GetHash() const
{
    SignedBlock block(*this);
    block.isBroadcasted = false;
    block.peerList.clear();
    return (CHashWriter{PROTOCOL_VERSION} << *this).GetHash();
}
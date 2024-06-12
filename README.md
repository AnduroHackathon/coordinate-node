Coordinate Sidechain integration/staging tree
=====================================

What is Coordinate
------------------
Coordinate is a Bitcoin Layer 2 Sidechain that support digital assets natively though hash representations and
on-chain data blobs. (_images_, _audio_, _text_, etc ...) A user can interact with the Coordinate layer 2 by
pegging in (_depositing_) BTC or TBTC with the sidechain federation. These funds are secured by a 66% super majority.
You will get CBTC (_coordinate bitcoin_) that are redeemable to the federation at a 1:1 ratio for Bitcoin.

CBTC(s) will allow you to perfom various types of transactions on the Coordinate sidechain. You can perform
any Bitcoin like transaction on the Coordinate sidecahin. _E.x. P2PK, P2PKH, P2SH, P2WPKH, P2WSH, P2TR, etc ...
This is useful for users who desire more frequent settlement and on average lower fees than what Bitcoins mainnet
can offer. The second type is Coordinate's version 10 transactions the creation of onchain assets.


What is the Coordinate sidechain node?
--------------------------------------
The Coordinate sidechain node implementaton is the reference client to interact with the Coordinate sidechain.

Building
--------
```
git clone git@github.com:MarathonDH/coordinate-sidechain-node.git coordinate-sidechain-node
cd coordinate-sidechain-node/depends
export HOST_TRIPLET=$(./config.guess)
make HOST=$HOST_TRIPLET
cd ..
./autogen.sh
./configure --prefix=$PWD/depends/$HOST_TRIPLET
make
```

License
-------

The Coordinate Sidechain node is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.

Development Process
-------------------

The `master` branch is regularly built (see `doc/build-*.md` for instructions) and tested, but it is not guaranteed to be
completely stable. [Tags](https://github.com/bitcoin/bitcoin/tags) are created
regularly from release branches to indicate new official, stable release versions of Bitcoin Core.

The https://github.com/bitcoin-core/gui repository is used exclusively for the
development of the GUI. Its master branch is identical in all monotree
repositories. Release branches and tags do not exist, so please do not fork
that repository unless it is for development reasons.

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md)
and useful hints for developers can be found in [doc/developer-notes.md](doc/developer-notes.md).

Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test on short notice. Please be patient and help out by testing
other people's pull requests, and remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing

Developers are strongly encouraged to write [unit tests](src/test/README.md) for new code, and to
submit new unit tests for old code. Unit tests can be compiled and run
(assuming they weren't disabled in configure) with: `make check`. Further details on running
and extending unit tests can be found in [/src/test/README.md](/src/test/README.md).

There are also [regression and integration tests](/test), written
in Python.
These tests can be run (if the [test dependencies](/test) are installed) with: `test/functional/test_runner.py`

The CI (Continuous Integration) systems make sure that every pull request is built for Windows, Linux, and macOS,
and that unit/sanity tests are run automatically.

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.

Translations
------------

Changes to translations as well as new translations can be submitted to
[Bitcoin Core's Transifex page](https://www.transifex.com/bitcoin/bitcoin/).

Translations are periodically pulled from Transifex and merged into the git repository. See the
[translation process](doc/translation_process.md) for details on how this works.

**Important**: We do not accept translation changes as GitHub pull requests because the next
pull from Transifex would automatically overwrite them again.

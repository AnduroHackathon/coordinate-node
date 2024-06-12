ASSET CREATION

1) createrawtransaction `createrawtransaction "[{\"txid\":\"myid\",\"vout\":0}]" "[{\"address\":0.01}]"`

The createrawtransaction RPC method allows you to create a new raw transaction by specifying inputs and outputs.

## Parameters:

* Inputs: Previous transaction outputs to spend.
* Outputs: Addresses or data to create.
* Locktime: Raw locktime. A non-zero value activates locktime on inputs.
* Replaceable: Marks the transaction as BIP125-replaceable. Default is false.
* Asset Parameters: Additional parameters for asset creation, including asset type, ticker, headline, payload, and payload data.

## Asset Creation Outputs:
Three specific outputs are added for asset creation:

* Controller output
* Supply
* Change output

## Asset Parameters:
Additional parameters have been added for asset creation:

Asset Type: Specifies the type of asset being created.

Ticker: Represents the symbol of the asset.

Headline: Describes the title of the asset.

Payload: Refers to additional information associated with the asset and SHA256 of payloaddata.

## Example :
a886c412c18aa59993daff26d3a1ae76d513a399e411a06f4ed16a9f20029c28

Payload Data: Represents the payload data in hexadecimal format, converted from string using a hex-string converter
`https://codebeautify.org/hex-string-converter`

## Example:
Hexadecimal format:
5b7b22696d6167655f75726c223a2268747470733a2f2f63646e2e66726565636f646563616d702e6f72672f706c6174666f726d2f756e6976657273616c2f6170706c652d73746f72652d62616467652e737667227d5d

Converted string:
[{"image_url":"https://cdn.freecodecamp.org/platform/universal/apple-store-badge.svg"}]

## Result:
A hex-encoded raw transaction.

2) signrawtransactionwithwallet `signrawtransactionwithwallet "myhex"`

The signrawtransactionwithwallet RPC method is used to sign the transaction using the wallet's private keys. It takes the hex-encoded raw transaction as input and returns the signed transaction in hexadecimal format.

## Result:
"hex" (string) The signed transaction hash in hex.

3) sendrawtransaction `sendrawtransaction "signedhex"`

Once the transaction is signed, the sendrawtransaction RPC method is used to broadcast the signed transaction to the network.

## Result:
(string) The transaction hash in hex, indicating that the transaction has been successfully broadcasted.




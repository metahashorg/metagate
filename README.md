# MetaGate

This repository contains Metahash wallet c++ source code. Wallet supports following currencies: MHC, BTC, ETC.

## Get the source code
Clone the repository by:
```shell
git clone https://github.com/metahashorg/metagate
```

## Build

Current Qt version - 10.1. For Windows, Qt 10.1 with visual studio 2015 compiler.
Detailed instructions for the project build on linux, mac or win can be found in the [deploy folder](https://github.com/metahashorg/metagate/tree/master/deploy).

## Api to connect with javascript

When `Q_INVOKABLE` qt-function returns some result, in javascript it must be got via callback, e.g.: 
```shell
mainWindow.openFolderDialog(beginPath, caption, function(returnValue) {
alert(returnValue);
});
```

### How to work with TMH wallets

```shell
Q_INVOKABLE void createWallet(QString requestId, QString password);`
# Generates Metahash wallet, puts this generated wallet to ~/.metahash_wallets/ named address. 
# javascript is called after completion of this function
createWalletResultJs(requestid, publickey, address, exampleMessage, signature, errorNum, errorMessage, fullKeyPath)

Q_INVOKABLE QString getAllWalletsJson();
# Gets the list of all metahash accounts. 
# Result returns as a json array

Q_INVOKABLE QString getAllWalletsAndPathsJson();
# Gets the list of all metahash accounts. Result returns as a json array [{"address":"addr","path":"path"}]

Q_INVOKABLE void signMessage(QString requestId, QString address, QString text, QString password);
# Message's signing.
# javascript is called after completion of this function 
signMessageResultJs(requestId, signature, publicKey, errorNum, errorMessage)

Q_INVOKABLE void signMessageV2(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex);
# signs the transaction in a new binary format
# value, fee, nonce - a string with decimal number
# Result returns to 
signMessageV2ResultJs(requestId, signature, publicKey, txHex, errorNum, errorMessage)

Q_INVOKABLE void signMessageV3(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex, QString paramsJson);
# Sends transaction with C++ (like signMessageDelegate, but differs because of the data field)
# Result returns to 
# if nonce empty, then calc nonce
signMessageV3ResultJs(requestId, "Ok/Not ok", errorNum, errorMessage)

Q_INVOKABLE void signMessageDelegate(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString valueDelegate, QString paramsJson);
# signs the transaction in a new binary format with delegate data
# if nonce empty, then calc nonce
paramsJson - json of {"countServersSend": 3, "countServersGet": 3, "typeSend": "proxy", "typeGet": "torrent", "timeout_sec": 6} type
# Result returns to 
signMessageDelegateResultJs(requestId, "Ok/Not ok", errorNum, errorMessage)

Q_INVOKABLE void signMessageUnDelegate(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString valueDelegate, QString paramsJson);
# signs the transaction in a new binary format with delegate data
# if nonce empty, then calc nonce
paramsJson - json of {"countServersSend": 3, "countServersGet": 3, "typeSend": "proxy", "typeGet": "torrent", "timeout_sec": 6} type
# Result returns to 
signMessageUnDelegateResultJs(requestId, "Ok/Not ok", errorNum, errorMessage)

Q_INVOKABLE void checkAddress(QString requestId, QString address);
# To check the address for correctness. The result will return to the function:
checkAddressResultJs(requestId, "ok"/"not valid", errorNum, errorMessage)

Q_INVOKABLE void getOnePrivateKey(QString requestId, QString keyName, bool isCompact);
# Returns private key to the function
getOnePrivateKeyResultJs(requestId, key, errorNum, errorMessage)
# The isCompact parameter is used if you need a more compact format.

Q_INVOKABLE void createRsaKey(QString requestId, QString address, QString password);
# Generates rsa key for specified address.
# javascript is called after completion of this function 
createRsaKeyResultJs(requestId, publicKeyHex, errorNum, errorMessage)

Q_INVOKABLE void getRsaPublicKey(QString requestId, QString address);
# Returns public key from rsa key
getRsaPublicKeyResultJs(requestId, publicKeyHex, errorNum, errorMessage)

Q_INVOKABLE void encryptMessage(QString requestId, QString publicKeyHex, QString message);
# Encrypt message generated via rsa public key
# javascript is called after completion of this function 
encryptMessageResultJs(requestId, message, errorNum, errorMessage)

Q_INVOKABLE void decryptMessage(QString requestId, QString addr, QString password, QString encryptedMessageHex);
# Decrypts message generated via rsa key
# javascript is called after completion of this function 
decryptMessageResultJs(requestId, message, errorNum, errorMessage)

Q_INVOKABLE void saveRawPrivKey(QString requestId, QString rawPrivKey, QString password);
# Saves private key
# rawPrivKey - unencrypted private key
# javascript is called after completion of this function 
saveRawPrivkeyResultJs(requestId, address, errorNum, errorMessage)
```

### How to work with MHC Metahash wallets

```shell
Q_INVOKABLE void createWalletMHC(QString requestId, QString password);
# Generates Metahash wallet, puts this generated wallet to ~/.metahash_wallets/ named address
# javascript is called after completion of this function 
createWalletMHCResultJs(requestid, publickey, address, exampleMessage, signature, errorNum, errorMessage, fullKeyPath)

Q_INVOKABLE QString getAllMHCWalletsJson();
# Gets the list of all metahash accounts. 
# Result returns as a json array

Q_INVOKABLE QString getAllMHCWalletsAndPathsJson();
# Gets the list of all metahash accounts. 
# Result returns as a json array [{"address":"addr","path":"path"}]

Q_INVOKABLE void signMessageMHC(QString requestId, QString address, QString text, QString password);
# Message's signing.
# javascript is called after completion of this function 
signMessageMHCResultJs(requestId, signature, publicKey, errorNum, errorMessage)

Q_INVOKABLE void signMessageMHCV2(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex);
# signs the transaction in a new binary format
# value, fee, nonce - a string with decimal number
# Result returns to 
signMessageMHCV2ResultJs(requestId, signature, publicKey, txHex, errorNum, errorMessage)

Q_INVOKABLE void signMessageMHCV3(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex, QString paramsJson);
#  Sends transaction with C++ (like signMessageDelegate, but differs because of the data field)
# if nonce empty, then calc nonce
# Result returns to 
signMessageMHCV3ResultJs(requestId, "Ok/Not ok", errorNum, errorMessage)

Q_INVOKABLE void signMessageMHCDelegate(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString valueDelegate, QString paramsJson);
# signs the transaction in a new binary format with delegate data
# if nonce empty, then calc nonce
paramsJson - json of {"countServersSend": 3, "countServersGet": 3, "typeSend": "proxy", "typeGet": "torrent", "timeout_sec": 6} type
# Result returns to 
signMessageDelegateMhcResultJs(requestId, "Ok/Not ok", errorNum, errorMessage)

Q_INVOKABLE void signMessageMHCUnDelegate(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString valueDelegate, QString paramsJson);
# signs the transaction in a new binary format with delegate data
# if nonce empty, then calc nonce
paramsJson - json of {"countServersSend": 3, "countServersGet": 3, "typeSend": "proxy", "typeGet": "torrent", "timeout_sec": 6} type
# Result returns to 
signMessageUnDelegateMhcResultJs(requestId, "Ok/Not ok", errorNum, errorMessage)

Q_INVOKABLE void checkAddress(QString requestId, QString address);
# To check the address for correctness. The result will return to the function:
checkAddressResultJs(requestId, "ok"/"not valid", errorNum, errorMessage)

Q_INVOKABLE void getOnePrivateKeyMHC(QString requestId, QString keyName, bool isCompact);
# Returns private key to the function
getOnePrivateKeyMHCResultJs(requestId, key, errorNum, errorMessage)
# The isCompact parameter is used if you need a more compact format.

Q_INVOKABLE void saveRawPrivKeyMHC(QString requestId, QString rawPrivKey, QString password);
# Saves private key
# rawPrivKey - unencrypted private key
# javascript is called after completion of this function 
saveRawPrivkeyMHCResultJs(requestId, address, errorNum, errorMessage)

```

### How to work with Ethereum wallets

```shell
Q_INVOKABLE void createWalletEth(QString requestId, QString password);
# Generates Ethereum wallet, puts the file to ~/.metahash_wallets/eth/
# javascript is called after completion of this function 
createWalletEthResultJs(requestId, address, errorNum, errorMessage, fullKeyPath)

Q_INVOKABLE void signMessageEth(QString requestId, QString address, QString password, QString nonce, QString gasPrice, QString gasLimit, QString to, QString value, QString data);
# Generates the signed Ethereum transaction
# Parameters:
  # address — address of the account
  # password — password of the account
  Further parameters are hexadecimal values prefixed with 0x
# When transferring tokens, "to" should be the token address, "value" == 0x0 (commonly), recipient and amount of tokens must be encoded in the data parameter
# Usually data parameter is equal to 0x
# javascript is called after completion of this function 
signMessageEthResultJs(requestId, result, errorNum, errorMessage)

Q_INVOKABLE QString getAllEthWalletsJson();
# Gets the list of all ethereum accounts. 
# Result returns as a json array

Q_INVOKABLE QString getAllEthWalletsAndPathsJson();
# Gets the list of all metahash accounts. 
# Result returns as a json array
[{"address":"addr","path":"path"}]

Q_INVOKABLE void checkAddressEth(QString requestId, QString address);
# To check the address for correctness. The result will return to the function:
checkAddressEthResultJs(requestId, "ok"/"not valid", errorNum, errorMessage)

Q_INVOKABLE void getOnePrivateKeyEth(QString requestId, QString keyName);
# Returns private key to the function
getOnePrivateKeyEthResultJs(requestId, key, errorNum, errorMessage)

```

### How to work with Bitcoin wallet

```shell
Q_INVOKABLE void createWalletBtc(QString requestId);
Q_INVOKABLE void createWalletBtcPswd(QString requestId, QString password);
# Generates Bitcoin wallet, puts file to ~/.metahash_wallets/btc/
# javascript is called after completion of this function 
createWalletBtcResultJs(requestId, address, errorNum, errorMessage, fullKeyPath)

Q_INVOKABLE void signMessageBtc(QString requestId, QString address, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees);
Q_INVOKABLE void signMessageBtcPswd(QString requestId, QString address, QString password, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees);
# Generating Bitcoin transaction
# Parameters:
  # jsonInputs - utxos in the format [{"tx_hash": "string", "tx_index": figure, "scriptPubKey": "string", "value": "figure in the string in decimal format"}]
  # value - is needed for sending. Possible variants: "all" or decimal number
  # fees - Possible variants: "auto" or decimal number
  # estimateComissionInSatoshi if there was "auto", here must be specified a decimal number
# javascript is called after completion of this function 
signMessageBtcResultJs(requestId, result, errorNum, errorMessage)

Q_INVOKABLE void signMessageBtcPswdUsedUtxos(QString requestId, QString address, QString password, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees, QString jsonUsedUtxos);
# Generating Bitcoin transaction
# Parameters:
  # jsonInputs - utxos in the format [{"tx_hash": "string", "tx_index": figure, "scriptPubKey": "string", "value": "figure in the string in decimal format"}]
  # value - is needed for sending. Possible variants: "all" or decimal number
  # fees - Possible variants: "auto" or decimal number
  # estimateComissionInSatoshi if there was "auto", here must be specified a decimal number
  # jsonUsedUtxos performed transactions. Empty array or value from the previous function call result
# javascript is called after completion of this function 
signMessageBtcUsedUtxosResultJs(requestId, transaction, jsonUsedUtxos, transactionHash, errorNum, errorMessage)

Q_INVOKABLE QString getAllBtcWalletsJson();
# Gets the list of all bitcoin accounts. 
# Result returns as a json array

Q_INVOKABLE QString getAllBtcWalletsAndPathsJson();
# Gets the list of all metahash accounts. 
# Result returns as a json array [{"address":"addr","path":"path"}]

Q_INVOKABLE void checkAddressBtc(QString requestId, QString address);
# To check the address for correctness. The result will return to the function:
checkAddressBtcResultJs(requestId, "ok"/"not valid", errorNum, errorMessage)

Q_INVOKABLE void getOnePrivateKeyBtc(QString requestId, QString keyName);
# Returns private key to the function
getOnePrivateKeyBtcResultJs(requestId, key, errorNum, errorMessage)

```

### General functions

```shell
Q_INVOKABLE void savePrivateKeyAny(QString requestId, QString privateKey, QString password);
# Tries to recognize a key type and saves it to the right place
# After the operation is completed, it calls function
savePrivateKeyAnyResultJs(requestId, "ok", errorNum, errorMessage)

Q_INVOKABLE void updateAndReloadApplication();
# Restarts wallet installing updates

Q_INVOKABLE void qtOpenInBrowser(QString url);
# Opens link in default browser

Q_INVOKABLE void getWalletFolders();
# Function calls javascript
walletFoldersJs(walletDefaultPath, walletCurrentPath, userName, errorNum, errorMessage)

Q_INVOKABLE void setPaths(QString newPatch, QString newUserName);
# Set current walletPath.  newUserName to the path  has not to be written! It is needed for reference only.
# javascript is called after completion of this function
setPathsJs(result("Ok", "Not ok"), errorNum, errorMessage)

Q_INVOKABLE QString openFolderDialog(QString beginPath, QString caption);
# Shows to user the dialog box providing the ability to select a directory. 
beginPath # initial directory location
caption # name of the dialog box
#  Returns the path to the selected by user directory or an empty string, if user declines selecting directory.

Q_INVOKABLE bool migrateKeysToPath(QString newPath);
# Moves keys from the previous user's directory (home/user/.metahash_wallets) to the specified directory.

Q_INVOKABLE void exitApplication();
# Stops the app.

Q_INVOKABLE void restartBrowser();
# restart browser

Q_INVOKABLE QString backupKeys(QString caption);
# Backs up keys to the file. Before backup, user is shown a dialog box providing ability to select a path.
caption # name of the dialog 
# Returns error description or empty string, if successful.

Q_INVOKABLE QString restoreKeys(QString caption);
# Recovers keys from a file. Before recovery, user is shown a dialog box providing ability to select backup file.
caption # name of the dialog
#  Returns error description or empty string, if successful.

Q_INVOKABLE void getMachineUid();
# Returns unique machine ID to the machineUidJs(uid) function.

Q_INVOKABLE void setUserName(const QString &userName);
# Sets username for the user button.

Q_INVOKABLE void setHasNativeToolbarVariable()
# Sets window.hasNativeToolbar javascript variable to true.

Q_INVOKABLE void setCommandLineText(const QString &text);
# Sets text to command line.

Q_INVOKABLE void openWalletPathInStandartExplorer();
# Open directory containing keys in standard explorer.

Q_INVOKABLE void setPagesMapping(QString mapping);
# Set up correspondence between metagate links and pages. Format:
{
  "routes": 
  [
    {
      "url":"login.html", 
      "name":"/MetaGate/Login",
      "isExternal":false
    },
    {
      "url":"login.html", 
      "name":"/MetaGate/Login",
      "isExternal":true
     }
  ]
}

Q_INVOKABLE void getIpsServers(QString requestId, QString type, int length, int count);
# Request ip list from top to ping
# type node type: torrent, proxy
# length restriction to the array. Elements will be selected from the range [0, length — 1]
# count - the amount of return elements.
# After completion of this function, response will arrive to getIpsServersJs(requestId, json, errorInt, errorString)
# Json format: "[\"127.0.0.1:9999\", \"127.0.0.1:9999\"]"

Q_INVOKABLE void lineEditReturnPressed(QString text);
# Emulate text input to lineEdit.

Q_INVOKABLE void saveFileFromUrl(QString url, QString saveFileWindowCaption, QString fileName, bool openAfterSave);
# Shows file save dialog, then downloads the file and saves it.

Q_INVOKABLE void chooseFileAndLoad(QString requestId, QString openFileWindowCaption, QString fileName);
# Shows the file selection dialog, then encodes the selected file in base64 and sends it to the function
loadFileResultJs(requestId, result, errorNum, errorMessage)

Q_INVOKABLE void getAppInfo(const QString requestId);
# Returns version of application and other information. 
# Result returns to the function:
getAppInfoResultJs(requestId, isProduction, version, gitCommit);

# When changing the contents of one of the directories with keys, the following function will be called
directoryChangedResultJs(path, ethName);

Q_INVOKABLE void qrEncode(QString requestId, QString textHex);
# Encodes text to qr code
# Result returns to the function:
qrEncodeResultJs(requestId, result);
# where result is a png image in base64

Q_INVOKABLE void qrEncode(QString requestId, QString pngBase64);
# Decodes text from qr code
# Result returns to the function:
qrDecodeResultJs(requestId, result);
# where result is a text in base16

Q_INVOKABLE void metaOnline();
# Request for statistics on being application online
# Result returns to the function:
onlineResultJs(jsonResult, errorNum, errorMessage);

```


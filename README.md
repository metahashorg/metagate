# MetaGate

This repository contains Metahash wallet c++ source code. Wallet supports following currencies: MHC, BTC, ETC.

## Get the source code
Clone the repository by:
```shell
git clone https://github.com/metahashorg/metagate
```

## Build

Detailed instructions for the project build on linux, mac or win can be found in the [deploy folder](https://github.com/metahashorg/metagate/tree/master/deploy).

```
Api для общения с javascript

Если qt-функция Q_INVOKABLE возвращает какой-то результат, то в javascript его нужно ловить через callback, например так
mainWindow.openFolderDialog(beginPath, caption, function(returnValue) {
alert(returnValue);
});


Работа с TMH Metahash кошельками

Q_INVOKABLE void createWallet(QString requestId, QString password);
Создает metahash кошелек, ложит созданный кошелек в ~/.metahash_wallets/ с именем address
По окончанию работы функции вызывается javascript
createWalletResultJs(requestid, publickey, address, exampleMessage, signature, errorNum, errorMessage, fullKeyPath)

Q_INVOKABLE QString getAllWalletsJson();
Получает список всех metahash аккаунтов.
Результат возвращается в виде json массива

Q_INVOKABLE QString getAllWalletsAndPathsJson();
Получает список всех metahash аккаунтов.
Результат возвращается в виде json массива [{"address":"addr","path":"path"}]

Q_INVOKABLE void signMessage(QString requestId, QString address, QString text, QString password);
Подпись сообщения.
По окончанию работы функции вызывается javascript
signMessageResultJs(requestId, signature, publicKey, errorNum, errorMessage)

Q_INVOKABLE void createRsaKey(QString requestId, QString address, QString password);
Создает rsa ключ для указанного адреса.
По окончанию работы функции вызывается javascript
createRsaKeyResultJs(requestId, publicKeyHex, errorNum, errorMessage)

Q_INVOKABLE void decryptMessage(QString requestId, QString addr, QString password, QString encryptedMessageHex);
Расшифровывает сообщение, созданное rsa ключем
По окончанию работы функции вызывается javascript
decryptMessageResultJs(requestId, message, errorNum, errorMessage)


Работа с MHC Metahash кошельками

Q_INVOKABLE void createWalletMHC(QString requestId, QString password);
Создает metahash кошелек, ложит созданный кошелек в ~/.metahash_wallets/ с именем address
По окончанию работы функции вызывается javascript
createWalletMHCResultJs(requestid, publickey, address, exampleMessage, signature, errorNum, errorMessage, fullKeyPath)

Q_INVOKABLE QString getAllMHCWalletsJson();
Получает список всех metahash аккаунтов.
Результат возвращается в виде json массива

Q_INVOKABLE QString getAllMHCWalletsAndPathsJson();
Получает список всех metahash аккаунтов.
Результат возвращается в виде json массива [{"address":"addr","path":"path"}]

Q_INVOKABLE void signMessageMHC(QString requestId, QString address, QString text, QString password);
Подпись сообщения.
По окончанию работы функции вызывается javascript
signMessageMHCResultJs(requestId, signature, publicKey, errorNum, errorMessage)


Работа с Ethereum кошельками

Q_INVOKABLE void createWalletEth(QString requestId, QString password);
Создает Ethereum wallet. Файл кладет в ~/.metahash_wallets/eth/
По окончанию работы функции вызывается javascript
createWalletEthResultJs(requestId, address, errorNum, errorMessage, fullKeyPath)

Q_INVOKABLE void signMessageEth(QString requestId, QString address, QString password, QString nonce, QString gasPrice, QString gasLimit, QString to, QString value, QString data);
Создает подписанную Ethereum транзакцию.
Параметры:
address - адрес аккаунта
password - пароль аккаунта
Дальнейшие параметры - это шестнадцатеричные значения, начинающиеся с 0x.
В случае отправки токенов, to должно быть адресом токена, value == 0x0 (обычно), получатель токена и количество токенов должны быть закодированы в параметре data
В случае обычной транзакции параметр data, как правило равен 0x
По окончанию работы функции вызывается javascript
signMessageEthResultJs(requestId, result, errorNum, errorMessage)

Q_INVOKABLE QString getAllEthWalletsJson();
Получает список всех ethereum аккаунтов.
Результат возвращается в виде json массива

Q_INVOKABLE QString getAllEthWalletsAndPathsJson();
Получает список всех metahash аккаунтов.
Результат возвращается в виде json массива [{"address":"addr","path":"path"}]



Работа с Bitcoin кошельком

Q_INVOKABLE void createWalletBtc(QString requestId);
Q_INVOKABLE void createWalletBtcPswd(QString requestId, QString password);
Создает Bitcoin wallet. Файл кладет в ~/.metahash_wallets/btc/
По окончанию работы функции вызывается javascript
createWalletBtcResultJs(requestId, address, errorNum, errorMessage, fullKeyPath)

Q_INVOKABLE void signMessageBtc(QString requestId, QString address, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees);
Q_INVOKABLE void signMessageBtcPswd(QString requestId, QString address, QString password, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees);
Генерация bitcoin транзакции
Параметры:
jsonInputs - utxos в формате [{"tx_hash": "string", "tx_index": число, "scriptPubKey": "string", "value": "число в строке 10-м формате"}]
value значение для отправки. Возможные варианты: "all" или десятичное число
fees Размер комиссии. Возможные варианты "auto" или десятичное число
estimateComissionInSatoshi в случае, если предыдущий вариант auto, должно быть указано десятичное число
По окончанию работы функции вызывается javascript
signMessageBtcResultJs(requestId, result, errorNum, errorMessage)

Q_INVOKABLE QString getAllBtcWalletsJson();
Получает список всех bitcoin аккаунтов.
Результат возвращается в виде json массива

Q_INVOKABLE QString getAllBtcWalletsAndPathsJson();
Получает список всех metahash аккаунтов.
Результат возвращается в виде json массива [{"address":"addr","path":"path"}]
```

Общие функции
```shell
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
```

```shell
Q_INVOKABLE void setPagesMapping(QString mapping);
Установить соответствие между ссылками metagate и страницами. Формат
{"routes":[{"url":"login.html", "name":"/MetaGate/Login""isExternal":false},{"url":"login.html", "name":"/MetaGate/Login""isExternal":true}]}

Q_INVOKABLE void getIpsServers(QString requestId, QString type, int length, int count);
Запросить список ip из топа по пингу
type тип ноды: torrent, proxy
length ограничение на массив. Элементы будут выбираться из диапазона [0, length - 1]
count количество возвращаемых элементов.
После выполнения функции ответ придет в 
getIpsServersJs(requestId, json, errorInt, errorString)
Формат json-а
"[\"206.189.14.22:9999\", \"206.189.14.52:9999\"]"

```

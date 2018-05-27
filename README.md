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


Работа с Metahash кошельками

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


Работа с TMH кошельками

Q_INVOKABLE void createWalletTMH(QString requestId, QString password);
Создает metahash кошелек, ложит созданный кошелек в ~/.metahash_wallets/ с именем address
По окончанию работы функции вызывается javascript
createWalletTmhResultJs(requestid, publickey, address, exampleMessage, signature, errorNum, errorMessage, fullKeyPath)

Q_INVOKABLE QString getAllTMHWalletsJson();
Получает список всех metahash аккаунтов.
Результат возвращается в виде json массива

Q_INVOKABLE QString getAllTMHWalletsAndPathsJson();
Получает список всех metahash аккаунтов.
Результат возвращается в виде json массива [{"address":"addr","path":"path"}]

Q_INVOKABLE void signMessageTMH(QString requestId, QString address, QString text, QString password);
Подпись сообщения.
По окончанию работы функции вызывается javascript
signMessageTmhResultJs(requestId, signature, publicKey, errorNum, errorMessage)


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


Общие функции

Q_INVOKABLE void updateAndReloadApplication();
Перезапускает кошелек с установкой обновлений

Q_INVOKABLE void qtOpenInBrowser(QString url);
Открывает ссылку в браузере по умолчанию

Q_INVOKABLE void getWalletFolders();
Функция вызывает javascript
walletFoldersJs(walletDefaultPath, walletCurrentPath, userName, errorNum, errorMessage)

Q_INVOKABLE void setPaths(QString newPatch, QString newUserName);
Установить текущий walletPath. newUserName к путю не прописывается! Нужен только для справочной информации
По окончанию работы функции вызывается javascript
setPathsJs(result("Ok", "Not ok"), errorNum, errorMessage)

Q_INVOKABLE QString openFolderDialog(QString beginPath, QString caption);
Показывает пользователю диалоговое окно с возможностью выбора каталога.
beginPath - первоначальное расположение каталога
caption - название диалогового окна
Возвращает путь к выбранному пользователем каталогу или пустую строку, если пользователь отказался от выбора каталога

Q_INVOKABLE bool migrateKeysToPath(QString newPath);
Переносит ключи из предыдущего каталога пользователя (home/user/.metahash_wallets) в указанный каталог

Q_INVOKABLE void exitApplication();
Останавливает приложение

Q_INVOKABLE QString backupKeys(QString caption);
Бэкапит ключи в файл. Перед бэкапом пользователю показывается диалог с возможностью выбора пути
caption - заголовок диалога
Возвращается описание ошибки или пустая строка в случае успеха

Q_INVOKABLE QString restoreKeys(QString caption);
Восстанавливает ключи из файла. Перед восстановлением пользователю показывается диалог с возможностью выбора файла бэкапа
caption - заголовок диалога
Возвращается описание ошибки или пустая строка в случае успеха

Q_INVOKABLE void getMachineUid();
Возвращает в функцию machineUidJs(uid) уникальный id машины

Q_INVOKABLE void setUserName(const QString &userName);
Устанавливает имя пользователя для кнопки user

Q_INVOKABLE void setHasNativeToolbarVariable()
устанавливает javascript переменную window.hasNativeToolbar в true

Q_INVOKABLE void setCommandLineText(const QString &text);
Устанавливает текст в command line

Q_INVOKABLE void openWalletPathInStandartExplorer();
открыть каталог с ключами в стандартном explorer-е

```

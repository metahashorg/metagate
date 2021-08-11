ALTER TABLE balance ADD tokenBlockNum INT8 NOT NULL DEFAULT 0;
CREATE TABLE tokens (tokenAddress TEXT PRIMARY KEY NOT NULL, type TEXT NOT NULL, symbol TEXT NOT NULL, name TEXT NOT NULL, decimals INTEGER NOT NULL, emission INT8 NOT NULL, owner TEXT NOT NULL );
CREATE TABLE tokenBalances ( id INTEGER PRIMARY KEY NOT NULL, address TEXT NOT NULL, tokenAddress TEXT NOT NULL, received TEXT NOT NULL, spent TEXT NOT NULL, value TEXT NOT NULL, countReceived INT8 NOT NULL, countSpent INT8 NOT NULL, countTxs INT8 NOT NULL );
CREATE UNIQUE INDEX tokenBalancesUniqueIdx ON tokenBalances (address, tokenAddress);

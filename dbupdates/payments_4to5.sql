DELETE FROM payments;
ALTER TABLE payments ADD ind INT8 NOT NULL DEFAULT 0;
CREATE TABLE balance (id INTEGER PRIMARY KEY NOT NULL, currency VARCHAR(100), address TEXT NOT NULL, received TEXT NOT NULL, spent TEXT NOT NULL, countReceived INT8 NOT NULL, countSpent INT8 NOT NULL, countTxs INT8 NOT NULL, currBlockNum INT8 NOT NULL, countDelegated INT8 NOT NULL, delegate TEXT NOT NULL, undelegate TEXT NOT NULL, delegated TEXT NOT NULL, undelegated TEXT NOT NULL, reserved TEXT NOT NULL, forged TEXT NOT NULL );

CREATE UNIQUE INDEX balanceUniqueIdx ON balance ( currency ASC, address ASC) ;

DROP INDEX paymentsUniqueIdx;
CREATE UNIQUE INDEX paymentsUniqueIdx ON payments ( currency ASC, address ASC, txid ASC, blockNumber ASC, ind ASC ) 

ALTER TABLE payments ADD blockHash TEXT NOT NULL DEFAULT '';
DROP INDEX paymentsUniqueIdx;
CREATE UNIQUE INDEX paymentsUniqueIdx ON payments ( currency ASC, address ASC, txid ASC, isInput ASC, blockNumber ASC );

ALTER TABLE payments ADD intStatus INTEGER DEFAULT 0;
DROP INDEX paymentsUniqueIdx;
CREATE UNIQUE INDEX paymentsUniqueIdx ON payments ( currency ASC, address ASC, txid ASC, isInput ASC, blockNumber ASC );

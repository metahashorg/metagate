ALTER TABLE payments ADD blockNumber INTEGER DEFAULT 0;
DROP INDEX old_index;
CREATE INDEX new_index ON demo(col1, col2);

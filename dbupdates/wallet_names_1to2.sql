ALTER TABLE info ADD type INTEGER NOT NULL DEFAULT 0;
DROP INDEX usersUniqueIdx;
CREATE UNIQUE INDEX usersUniqueIdx ON info (wallet_id ASC, user ASC, device ASC, currency ASC, type ASC)

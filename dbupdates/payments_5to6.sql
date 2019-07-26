CREATE TABLE currency (id INTEGER PRIMARY KEY NOT NULL, isMhc BOOLEAN NOT NULL, currency VARCHAR(100) NOT NULL );

CREATE UNIQUE INDEX currencyUniqueIdx ON currency (isMhc, currency);

package com.sos.scheduler.engine.kernel.database;

/**
 * @author Joacim Zschimmer
 */
public enum DbmsKind {
    dbms_unknown,
    dbms_access,
    dbms_oracle,
    dbms_oracle_thin,
    dbms_sql_server,
    dbms_mysql,
    dbms_sossql,
    dbms_postgresql,
    dbms_db2,
    dbms_firebird,
    dbms_sybase,
    dbms_h2;

    boolean isOracle() {
        return this == dbms_oracle || this == dbms_oracle_thin;
    }
}

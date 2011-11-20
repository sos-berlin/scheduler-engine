package com.sos.scheduler.engine.kernel.database;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.variable.UnmodifiableVariableSet;
import com.sos.scheduler.engine.kernel.scheduler.Subsystem;
import com.sos.scheduler.engine.kernel.cppproxy.DatabaseC;

import java.util.HashMap;
import java.util.Map;
import javax.persistence.EntityManager;
import javax.persistence.EntityManagerFactory;
import javax.persistence.Persistence;


@ForCpp
public class DatabaseSubsystem implements Subsystem {
    public static final String emptyIdInDatabase = "-";
    private static final String persistenceUnitName = "schedulerEngine";

    private EntityManagerFactory entityManagerFactory = null;
    private EntityManager entityManager = null;
    private final DatabaseC cppProxy;

    public DatabaseSubsystem(DatabaseC cppProxy) {
        this.cppProxy = cppProxy;
    }

    public final void close() {
        if (entityManagerFactory != null)
            entityManagerFactory.close();   // Schließt auch EntityManager
    }

    public final EntityManager getEntityManager() {
        if (entityManagerFactory == null)
            entityManagerFactory = Persistence.createEntityManagerFactory(persistenceUnitName, entityManagerProperties());
        if (entityManager == null)
            entityManager = entityManagerFactory.createEntityManager();
        return entityManager;
    }

    private Map<String,String> entityManagerProperties() {
        Map<String,String> result = new HashMap<String,String>();
        UnmodifiableVariableSet p = getProperties();
        result.put("javax.persistence.jdbc.driver", p.get("jdbc.driverClass"));
        result.put("javax.persistence.jdbc.url", p.get("path"));
        result.put("javax.persistence.jdbc.user", p.get("user"));
        result.put("javax.persistence.jdbc.password", p.get("password"));
        return result;
    }

    /** Liefert auch "password" */
    public final UnmodifiableVariableSet getProperties() {
        return cppProxy.properties().getSister();
    }

    public static String idForDatabase(String id) {
        return id.isEmpty()? emptyIdInDatabase : id;
    }
}

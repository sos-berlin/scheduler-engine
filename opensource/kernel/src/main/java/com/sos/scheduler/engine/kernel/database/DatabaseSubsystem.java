package com.sos.scheduler.engine.kernel.database;

import com.google.common.collect.ImmutableMap;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.cppproxy.DatabaseC;
import com.sos.scheduler.engine.kernel.scheduler.Subsystem;
import com.sos.scheduler.engine.kernel.variable.UnmodifiableVariableSet;

import javax.persistence.EntityManager;
import javax.persistence.EntityManagerFactory;
import javax.persistence.Persistence;


@ForCpp
public class DatabaseSubsystem implements Subsystem {
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

    private ImmutableMap<String,String> entityManagerProperties() {
        UnmodifiableVariableSet v = schedulerVariableSet();
        return new ImmutableMap.Builder<String,String>()
            .put("javax.persistence.jdbc.driver", v.get("jdbc.driverClass"))
            .put("javax.persistence.jdbc.url", v.get("path"))
            .put("javax.persistence.jdbc.user", v.get("user"))
            .put("javax.persistence.jdbc.password", v.get("password"))
            .build();
    }

    /** Liefert auch "password" */
    private UnmodifiableVariableSet schedulerVariableSet() {
        return cppProxy.properties().getSister();
    }
}

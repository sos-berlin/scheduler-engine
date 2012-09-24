package com.sos.scheduler.engine.kernel.database;

import com.google.common.collect.ImmutableMap;
import com.sos.scheduler.engine.common.Lazy;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.cppproxy.DatabaseC;
import com.sos.scheduler.engine.kernel.scheduler.Subsystem;
import com.sos.scheduler.engine.kernel.variable.UnmodifiableVariableSet;

import javax.persistence.EntityManager;
import javax.persistence.EntityManagerFactory;

import static com.sos.scheduler.engine.persistence.SchedulerDatabases.persistenceUnitName;
import static javax.persistence.Persistence.createEntityManagerFactory;

@ForCpp
public class DatabaseSubsystem implements Subsystem {
    private final DatabaseC cppProxy;
    private final Lazy<EntityManagerFactory> entityManagerFactoryLazy = new Lazy<EntityManagerFactory>() {
        @Override protected EntityManagerFactory compute() {
            return createEntityManagerFactory(persistenceUnitName, entityManagerProperties());
        }
    };
    private final Lazy<EntityManager> entityManager = new Lazy<EntityManager>() {
        @Override protected EntityManager compute() {
            return entityManagerFactoryLazy.get().createEntityManager();
        }
    };

    public DatabaseSubsystem(DatabaseC cppProxy) {
        this.cppProxy = cppProxy;
    }

    public final void close() {
        if (entityManagerFactoryLazy.isDefined())
            entityManagerFactoryLazy.get().close();   // Schließt auch EntityManager
    }

    public final EntityManager getEntityManager() {
        return entityManager.get();
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

package com.sos.scheduler.engine.kernel.database;

import com.google.common.collect.ImmutableMap;
import com.sos.scheduler.engine.common.Lazy;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.cppproxy.DatabaseC;
import com.sos.scheduler.engine.kernel.scheduler.Subsystem;
import com.sos.scheduler.engine.kernel.variable.UnmodifiableVariableSet;

import javax.persistence.EntityManagerFactory;
import javax.persistence.PersistenceException;

import static com.sos.scheduler.engine.persistence.SchedulerDatabases.persistenceUnitName;
import static javax.persistence.Persistence.createEntityManagerFactory;

@ForCpp
public class DatabaseSubsystem implements Subsystem {
    private final DatabaseC cppProxy;
    private final Lazy<EntityManagerFactory> entityManagerFactoryLazy = new Lazy<EntityManagerFactory>() {
        @Override protected EntityManagerFactory compute() {
            try {
                return createEntityManagerFactory(persistenceUnitName, entityManagerProperties());
            } catch (PersistenceException e) {
                throw new RuntimeException(e +". Cause: "+e.getCause(), e);     // Hibernate liefert nur nichtssagende Meldung "Unable to build EntityManagerFactory", ohne den interessanten Cause
            }
        }
    };

    public DatabaseSubsystem(DatabaseC cppProxy) {
        this.cppProxy = cppProxy;
    }

    public final void close() {
        if (entityManagerFactoryLazy.isDefined())
            entityManagerFactoryLazy.get().close();   // Schließt auch alle EntityManager
    }

    public final EntityManagerFactory entityManagerFactory() {
        return entityManagerFactoryLazy.get();
    }

    private ImmutableMap<String,String> entityManagerProperties() {
        UnmodifiableVariableSet v = cppDatabaseProperties();
        return new ImmutableMap.Builder<String,String>()
            .put("javax.persistence.jdbc.driver", v.get("jdbc.driverClass"))
            .put("javax.persistence.jdbc.url", v.get("path"))
            .put("javax.persistence.jdbc.user", v.get("user"))
            .put("javax.persistence.jdbc.password", v.get("password"))
          //.put("hibernate.show_sql", "true")
            .build();
    }

    /** Liefert auch "password" */
    private UnmodifiableVariableSet cppDatabaseProperties() {
        return cppProxy.properties().getSister();
    }
}

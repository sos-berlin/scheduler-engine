package com.sos.scheduler.engine.kernel.database;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.Subsystem;
import com.sos.scheduler.engine.kernel.VariableSet;
import com.sos.scheduler.engine.kernel.cppproxy.DatabaseC;
import java.util.HashMap;
import java.util.Map;
import javax.persistence.EntityManager;
import javax.persistence.EntityManagerFactory;
import javax.persistence.Persistence;


@ForCpp
public class DatabaseSubsystem implements Subsystem {
    public static final String emptyIdinDatabase = "-";
    private static final String persistenceUnitName = "schedulerEngine";

    private EntityManagerFactory entityManagerFactory = null;
    private EntityManager entityManager = null;
    private final DatabaseC cppProxy;
//    private final DatabaseConfiguration config;


    public DatabaseSubsystem(DatabaseC cppProxy) {
        this.cppProxy = cppProxy;
//        this.config = config(getProperties());
    }


//    private static DatabaseConfiguration config(VariableSet p) {
//        return new DatabaseConfiguration(); // Ist noch leer
//        String jdbcDriverClassName = p.getStrictly("jdbc.driver");
//        String url = p.getStrictly("path");
//        String userName = p.getStrictly("userName");
//        String password = p.getStrictly("password");
//        return new DatabaseConfiguration(jdbcDriverClassName, url, userName, password);
//    }
    
    
    public final void close() {
        if (entityManagerFactory != null)
            entityManagerFactory.close();   // Schließt auch EntityManager
//        try {
//            if (connection != null)
//                connection.close();
//        } catch (SQLException x) { throw new SchedulerException(x); }
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
        VariableSet p = getProperties();
        result.put("javax.persistence.jdbc.driver", p.getStrictly("jdbc.driverClass"));
        result.put("javax.persistence.jdbc.url", p.getStrictly("path"));
        result.put("javax.persistence.jdbc.user", p.getStrictly("user"));
        result.put("javax.persistence.jdbc.password", p.getStrictly("password"));
        return result;
    }


    /** Liefert auch "password" */
    public final VariableSet getProperties() {
        return cppProxy.properties().getSister();
    }


    public static String idForDatabase(String id) {
        return id.isEmpty()? emptyIdinDatabase : id;
    }

//    public final Connection getConnection() {
//        if (connection != null)  connection = openConnection();
//        return connection;
//    }
//
//
//    private Connection openConnection() {
//        try {
//            return DriverManager.getConnection(config.getUrl(), config.getUserName(), config.getPassword());
//        } catch (SQLException x) { throw new SchedulerException(x); }
//    }

    
//    public final DatabaseConfiguration getConfiguration() {
//        return config;
//    }
}

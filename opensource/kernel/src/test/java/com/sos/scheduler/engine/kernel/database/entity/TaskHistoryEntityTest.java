package com.sos.scheduler.engine.kernel.database.entity;

import java.util.List;
import java.util.Properties;
import javax.persistence.*;
import org.junit.Ignore;
import org.junit.Test;

/**
 *
 * @author Joacim Zschimmer
 */
public class TaskHistoryEntityTest {
    private static final String persistenceUnitName = "schedulerEngine";
    private static final String jdbcDriverClassName = "org.h2.Driver";
    private static final String jdbcUrl = "jdbc:h2:c:/sos/tmp/scheduler-database";
	private static EntityManagerFactory entityManagerFactory;


    @Ignore
    @Test public void test() throws Exception {
        entityManagerFactory = Persistence.createEntityManagerFactory(persistenceUnitName, persistenceProperties());
		EntityManager em = entityManagerFactory.createEntityManager();
        em.setProperty(persistenceUnitName, em);
        try {
            Query q = em.createQuery("select t from " + TaskHistoryEntity.class.getSimpleName() + " t");
            List<TaskHistoryEntity> resultList = (List<TaskHistoryEntity>)q.getResultList();
            for (TaskHistoryEntity e: resultList)  System.out.println(e);
            System.out.println("Size: " + resultList.size());
        } finally {
            em.close();
        }
    }


    private static Properties persistenceProperties() {
        Properties result = new Properties();
        result.setProperty("javax.persistence.jdbc.driver", jdbcDriverClassName);
        result.setProperty("javax.persistence.jdbc.url", jdbcUrl);
//        result.setProperty("javax.persistence.jdbc.user", "");
//        result.setProperty("javax.persistence.jdbc.password", "");
        return result;
    }
}

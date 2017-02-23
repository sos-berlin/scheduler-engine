package com.sos.scheduler.engine.kernel.persistence.hibernate

import com.sos.jobscheduler.data.scheduler.SchedulerId
import com.sos.scheduler.engine.data.job.{JobPath, JobPersistentState}
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import com.sos.scheduler.engine.kernel.persistence.hibernate.RichEntityManager.toRichEntityManager
import com.sos.scheduler.engine.persistence.SchedulerDatabases.schedulerIdToDatabase
import com.sos.scheduler.engine.persistence.entities.{JobEntity, JobEntityConverter}
import java.time.Duration
import javax.inject.{Inject, Singleton}
import javax.persistence.{EntityManager, EntityManagerFactory}

@Singleton
final class HibernateJobStore @Inject()(
    protected val schedulerId: SchedulerId,
    protected val clusterMemberId: ClusterMemberId,
    protected val entityManagerFactory: EntityManagerFactory)
extends AbstractHibernateStore[JobPersistentState, JobEntity]
with JobEntityConverter {

  def tryFetchAverageStepDuration(jobPath: JobPath)(implicit em: EntityManager): Option[Duration] = {
    val sql = """select sum({fn TIMESTAMPDIFF(SQL_TSI_SECOND, "START_TIME", "END_TIME")}) / sum("STEPS")"""+
        """ from SCHEDULER_HISTORY where "STEPS" > 0 and "SPOOLER_ID"=? and "JOB_NAME"=?"""
    em.useJDBCPreparedStatement(sql) { stmt =>
      stmt.setString(1, schedulerIdToDatabase(schedulerId))
      stmt.setString(2, jobPath.withoutStartingSlash)
      val resultSet = stmt.executeQuery()
      if (resultSet.next())
        Some(Duration.ofSeconds(resultSet.getLong(1)))
      else
        None
    }
  }
}

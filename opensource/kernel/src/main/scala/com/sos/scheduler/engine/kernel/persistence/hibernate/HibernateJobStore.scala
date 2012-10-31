package com.sos.scheduler.engine.kernel.persistence.hibernate

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.JobPersistent
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate.useJDBCPreparedStatement
import com.sos.scheduler.engine.persistence.SchedulerDatabases.schedulerIdToDatabase
import com.sos.scheduler.engine.persistence.entities.{JobEntityConverter, JobEntity}
import javax.inject.{Inject, Singleton}
import javax.persistence.{EntityManager, EntityManagerFactory}
import org.joda.time.Duration

@Singleton
final class HibernateJobStore @Inject()(
    protected val schedulerId: SchedulerId,
    protected val clusterMemberId: ClusterMemberId,
    protected val entityManagerFactory: EntityManagerFactory)
extends AbstractHibernateStore[JobPersistent, JobPath, JobEntity]
with JobEntityConverter {

  def tryFetchAverageStepDuration(jobPath: JobPath)(implicit em: EntityManager): Option[Duration] = {
    val sql = """select sum({fn TIMESTAMPDIFF(SQL_TSI_SECOND, "START_TIME", "END_TIME")}) / sum("STEPS")"""+
        """ from SCHEDULER_HISTORY where "STEPS" > 0 and "SPOOLER_ID"=? and "JOB_NAME"=?"""
    useJDBCPreparedStatement(sql) { stmt =>
      stmt.setString(1, schedulerIdToDatabase(schedulerId))
      stmt.setString(2, jobPath.withoutStartingSlash)
      val resultSet = stmt.executeQuery()
      if (resultSet.next())
        Some(new Duration(resultSet.getLong(1) * 1000))
      else
        None
    }
  }
}

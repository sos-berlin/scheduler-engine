package com.sos.scheduler.engine.kernel.persistence

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.JobPersistent
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import com.sos.scheduler.engine.kernel.persistence.ScalaJPA.useJDBCPreparedStatement
import com.sos.scheduler.engine.persistence.SchedulerDatabases.idForDatabase
import com.sos.scheduler.engine.persistence.entities.{JobEntityConverter, JobEntity}
import javax.inject.{Inject, Singleton}
import javax.persistence.{EntityManager, EntityManagerFactory}
import org.joda.time.Duration

@Singleton
final class JobStore @Inject()(
    protected val schedulerId: SchedulerId,
    protected val clusterMemberId: ClusterMemberId,
    protected val entityManagerFactory: EntityManagerFactory)
extends AbstractObjectJPAStore[JobPersistent, JobPath, JobEntity]
with JobEntityConverter {

  override def store(o: JobPersistent)(implicit em: EntityManager) {
    if (o.isDefault)
      delete(o.jobPath)   // Den normalen Zustand speichern wir nicht
    else
      super.store(o)
  }

  def tryFetchAverageStepDuration(jobPath: JobPath)(implicit em: EntityManager): Option[Duration] = {
    val sql = """select sum({fn TIMESTAMPDIFF(SQL_TSI_SECOND, "START_TIME", "END_TIME")}) / sum("STEPS")"""+
        """ from SCHEDULER_HISTORY where "STEPS" > 0 and "SPOOLER_ID"=? and "JOB_NAME"=?"""
    useJDBCPreparedStatement(sql) { stmt =>
      stmt.setString(1, idForDatabase(schedulerId))
      stmt.setString(2, jobPath.withoutStartingSlash)
      val resultSet = stmt.executeQuery()
      if (resultSet.next())
        Some(new Duration(resultSet.getLong(1) * 1000))
      else
        None
    }
  }
}

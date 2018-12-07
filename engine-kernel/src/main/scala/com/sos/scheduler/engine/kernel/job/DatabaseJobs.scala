package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.base.utils.ScalaUtils.RichAny
import com.sos.scheduler.engine.common.scalautil.AutoClosing._
import com.sos.scheduler.engine.common.scalautil.Collections.RichGenericCompanion
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.xml.VariableSets
import com.sos.scheduler.engine.data.job.{JobHistoryEntry, JobOverview, JobPath, ReturnCode, TaskId}
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.database.{DatabaseSubsystem, JdbcConnectionPool}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import java.sql
import java.sql.ResultSet
import javax.inject.{Inject, Singleton}
import scala.collection.immutable.Seq
import scala.concurrent.{Future, blocking}

/**
  * @author Joacim Zschimmer
  */
@Singleton
private[job] final class DatabaseJobs @Inject private(
  schedulerId: SchedulerId,
  databaseSubsystem: DatabaseSubsystem,
  schedulerConfiguration: SchedulerConfiguration,
  jdbcConnectionPool: JdbcConnectionPool,
  private[job] implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue) {

  def fetchJobHistory(jobPath: JobPath, limit: Int): Future[Seq[JobHistoryEntry]] =
    jdbcConnectionPool.readOnly { connection ⇒
      DatabaseJobs.fetchJobHistory(connection, jobHistoryQueryToSql(jobPath, limit))
    }

  def jobHistoryQueryToSql(jobPath: JobPath, limit: Int): String = {
    val select = new StringBuilder(1000)
    select ++= "select "
    if (limit < Int.MaxValue) select ++= s"%limit($limit) "
    //             1     2           3             4           5        6                    7        8            9      10           11            12       13            14
    select ++= """"ID", "JOB_NAME", "START_TIME", "END_TIME", "CAUSE", "CLUSTER_MEMBER_ID", "STEPS", "EXIT_CODE", "PID", "AGENT_URL", "PARAMETERS", "ERROR", "ERROR_CODE", "ERROR_TEXT" """
    select ++= " from "
    select ++= schedulerConfiguration.jobHistoryTableName
    select ++= """  where "JOB_NAME"="""
    select ++= DatabaseSubsystem.quoteSqlString(jobPath.withoutStartingSlash)
    select ++= """ and "SPOOLER_ID"="""
    select ++= DatabaseSubsystem.quoteSqlString(schedulerId.string.substitute("", "-"))
    select ++= """ order by "ID" desc"""
    inSchedulerThread {
      databaseSubsystem.transformSql(select.toString)
    }
  }
}

private[job] object DatabaseJobs {
  private[job] def fetchJobHistory(connection: sql.Connection, sqlStmt: String): Seq[JobHistoryEntry] =
    autoClosing(connection.prepareStatement(sqlStmt)) { stmt ⇒
      val resultSet = stmt.executeQuery()
      fetchJobHistory(connection, resultSet)
    }

  private def fetchJobHistory(connection: sql.Connection, resultSet: ResultSet): Seq[JobHistoryEntry] = {
    blocking {
      Vector.build[JobHistoryEntry] { builder ⇒
        while (resultSet.next()) {
          def readNonNull[A](name: String, op: String ⇒ A) =
            op(name) match {
              case null ⇒ sys.error(s"Unexpected null value in job history database table, field '$name'")
              case a ⇒ a
            }
          builder += JobHistoryEntry(
            taskId          = Option(resultSet.getInt(1)) map TaskId.apply,
            jobPath         = JobPath.makeAbsolute(readNonNull("JOB_NAME", resultSet.getString)),
            startedAt       = dateToInstant(readNonNull("START_TIME", resultSet.getDate)),
            endedAt         = Option(resultSet.getTimestamp(4)) map dateToInstant,
            cause           = Option(resultSet.getString(5)),
            clusterMemberId = Option(resultSet.getString(6)) map ClusterMemberId.apply,
            stepCount       = Option(resultSet.getInt(7)),
            returnCode      = Option(resultSet.getInt(8)) map ReturnCode.apply,
            pid             = Option(resultSet.getInt(9)),
            agentUri        = Option(resultSet.getString(10)),
            parameters      = Option(resultSet.getString(11)) map VariableSets.parseXml,
            error           = Option(resultSet.getBoolean(12)) filter identity map (_ ⇒
                                JobOverview.Error(
                                  code = Option(resultSet.getString(13)) getOrElse "",
                                  message = Option(resultSet.getString(14)) getOrElse "")))
        }
      }
    }
  }
}

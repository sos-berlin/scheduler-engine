package com.sos.scheduler.engine.tests.jira.js1926

import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.kernel.database.JdbcConnectionPool
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.{BeforeAndAfterAll, FreeSpec}

// This test works only if database.cxx creates a field "LONGEXTRA" as clob !!!
//@RunWith(classOf[JUnitRunner])
final class JS1926IT extends FreeSpec with BeforeAndAfterAll with ScalaSchedulerTest
{
  "set_history_field" in {
    pending
    val taskRun = startJob(JobPath("/JOB"))
    taskRun.result.await(99.s)
    val taskId = taskRun.taskId

    instance[JdbcConnectionPool].readOnly { connection ⇒
      val table = injector.instance[SchedulerConfiguration].jobHistoryTableName
      val sql = s"""select "ID", "SHORTEXTRA", "LONGEXTRA" from $table where "ID"=?"""
      autoClosing(connection.prepareStatement(sql)) { stmt ⇒
        stmt.setInt(1, taskId.number)
        val resultSet = stmt.executeQuery()
        if (!resultSet.next()) fail(s"Missing record in $table for $taskId")

        assert(resultSet.getInt("ID") == taskId.number)
        assert(resultSet.getString("SHORTEXTRA") == TestJob.shortString)
        assert(resultSet.getString("LONGEXTRA") == TestJob.longString)
      }
    } await 99.s
  }
}

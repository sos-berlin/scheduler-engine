package com.sos.scheduler.engine.tests.jira.js1176

import com.sos.scheduler.engine.client.command.HttpSchedulerCommandClient
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Futures._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPorts
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.log.{InfoLogEvent, WarningLogEvent}
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.SchedulerTestUtils.{awaitFailure, awaitSuccess}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.database.H2DatabaseServer
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1176.JS1176IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.{Future, Promise}

/**
 * JS-1176 BUG: If you change a job configuration while the database connection get lost then the JobScheduler can crash.
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1176IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val List(httpPort, databaseTcpPort) = findRandomFreeTcpPorts(2)
  private val databaseConfiguration = new H2DatabaseServer.Configuration {
    def directory = testEnvironment.databaseDirectory
    def tcpPort = databaseTcpPort
  }
  private lazy val databaseServer = new H2DatabaseServer(databaseConfiguration)
  private lazy val commandClient = instance[HttpSchedulerCommandClient]
  override protected lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(s"-tcp-port=$httpPort"),
    database = Some(databaseConfiguration))

  override protected def onBeforeSchedulerActivation(): Unit = {
    databaseServer.start()
  }

  onClose {
    databaseServer.close() // Nach dem Scheduler schließen, damit der beim Herunterfahren noch an die Datenbank herankommt.
  }

  private implicit def implicitCallQueue: SchedulerThreadCallQueue = instance[SchedulerThreadCallQueue]
  "Modifying a job while waiting for database should not crash" in {
    controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-303"))) {
      val waitingForDatabase = Promise[Unit]()
      eventBus.onHot[WarningLogEvent] {
        case e if e.codeOption == Some(MessageCode("SCHEDULER-958")) ⇒ // "Waiting 20 seconds before reopening the database"
          waitingForDatabase.trySuccess(())
      }
      databaseServer.stop()
      testEnvironment.fileFromPath(TestJobPath).append(" ")
      awaitResult(waitingForDatabase.future, ConfigurationDirectoryWatchPeriod + TestTimeout)
      val databaseStart = Future {
        Thread.sleep(5000)
        databaseServer.start()
      }
      assert(!databaseStart.isCompleted)
      awaitFailure(checkFolders()).getMessage should startWith ("SCHEDULER-184")  // "Scheduler database cannot be accessed due to a database problem"
      eventBus.awaitingEvent2[InfoLogEvent](timeout = LostDatabaseRetryTimeout + TestTimeout, predicate = _.codeOption == Some(MessageCode("SCHEDULER-807"))) {
        awaitSuccess(databaseStart)
      }
      awaitSuccess(checkFolders())
    }
  }

  private def checkFolders(): Future[String] = commandClient.execute(s"http://127.0.0.1:$httpPort", <check_folders/>)
}

private object JS1176IT {
  private val TestJobPath = JobPath("/test")
  private val LostDatabaseRetryTimeout = 60.s
  private val ConfigurationDirectoryWatchPeriod = 60.s
}

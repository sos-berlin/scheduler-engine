package com.sos.scheduler.engine.tests.jira.js1176

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.HasCloser.implicits._
import com.sos.scheduler.engine.common.scalautil.SideEffect._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPorts
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.log.WarningLogEvent
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.database.H2DatabaseServer
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1176.JS1176IT._
import com.sun.jersey.api.client.Client
import javax.ws.rs.core.MediaType.TEXT_XML_TYPE
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.{Await, Future, Promise}

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
  override protected lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(s"-tcp-port=$httpPort"),
    database = Some(databaseConfiguration))

  override protected def onBeforeSchedulerActivation(): Unit = {
    databaseServer.start()
  }

  closer(databaseServer)  // Nach dem Scheduler schließen, damit der beim Herunterfahren noch an die Datenbank herankommt.

  private implicit def implicitCallQueue: SchedulerThreadCallQueue = instance[SchedulerThreadCallQueue]

  "JS-1176" in {
    controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-303"))) {
      autoClosing(controller.newEventPipe()) { eventPipe ⇒
        val waitingPromise = Promise[Unit]()
        controller.getEventBus.onHot[WarningLogEvent] {
          case e if e.codeOption == Some(MessageCode("SCHEDULER-958")) ⇒ // "Waiting 20 seconds before reopening the database"
            waitingPromise.trySuccess(())
        }
        databaseServer.stop()
        testEnvironment.fileFromPath(TestJobPath).append(" ")
        Await.result(waitingPromise.future, ConfigurationDirectoryWatchPeriod + TestTimeout)
        val future = Future {
          Thread.sleep(5000)
          databaseServer.start()
        }
        assert(!future.isCompleted)
        val webClient = Client.create() sideEffect { o ⇒ onClose { o.destroy() } }
        webClient.resource(s"http://127.0.0.1:$httpPort").`type`(TEXT_XML_TYPE).post(<check_folders/>.toString())
        Await.result(future, LostDatabaseRetryTimeout + TestTimeout)
      }
    }
  }
}

private object JS1176IT {
  private val TestJobPath = JobPath("/test")
  private val LostDatabaseRetryTimeout = 60.s
  private val ConfigurationDirectoryWatchPeriod = 60.s
}


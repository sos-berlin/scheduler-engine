package com.sos.scheduler.engine.tests.jira.js1079

import JS1079IT._
import com.google.common.base.Charsets.UTF_8
import com.google.common.io.Files
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreePort
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.TaskEndedEvent
import com.sos.scheduler.engine.test.binary.CppBinariesDebugMode
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import java.io.File
import java.net._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner

/** JS-1079: Test des Supervisors mit vielen bestehenden TCP-Verbindungen.
  * Der Scheduler läuft im selben Prozess wie Scala, weshalb die Client-Sockets mitzählen.
  * Mehr Verbindungen werden möglich sein, wenn der Scheduler in einem eigenen Prozess gestartet wird.
  * <p>
  * /etc/security/limits.conf (Linux):
  * <pre>
  *   sos              hard    nofile          30000
  *   sos              soft    nofile          30000
  * </pre>
  */
@RunWith(classOf[JUnitRunner])
final class JS1079IT extends ScalaSchedulerTest {
  override protected lazy val testConfiguration = TestConfiguration(
    binariesDebugMode = Some(CppBinariesDebugMode.release))  // debug ist zu langsam
  private val nextUdpSocket = new UdpSocketGenerator
  private lazy val allDirectory = new File(controller.environment.configDirectory, "remote/_all")
  private lazy val serverTcpPort = findRandomFreePort(10000 until 20000)

  override def checkedBeforeAll() {
    val schedulerXmlString = schedulerXml(serverTcpPort).toString()
    controller.prepare()
    Files.write(schedulerXmlString, new File(controller.environment.configDirectory, "scheduler.xml"), UTF_8)
    allDirectory.mkdirs()
    super.beforeAll()
  }

  test(s"Configuration server with $connectionsMax permanently connected simulated clients (TCP, old v1.5 style)") {
    val clients = (1 to connectionsMax).par map { i => new SupervisorClient(i, nextUdpSocket(), new InetSocketAddress("127.0.0.1", serverTcpPort)) }
    for (c <- clients) {
      c.connect()
      c.registerMe()   // Gleich nach connect(), damit C++-Code nicht in "Nichts getan"-Schlaf fällt.
    }
    checkTasks()
    clients.head.fetchUpdatedFiles()   // Einmal, damit Supervisor.activate() aufgerufen wird
    Files.write("<job><script>exit 0</script></job>", new File(allDirectory, "test.job.xml"), UTF_8)   // Datei im Konfigurationsserver erzeugen, die ausgeteilt werden soll
    for (c <- clients) c expectUdp "<check_folders/>"
    for (c <- clients) c.close()
  }

  /** Auch bei den vielen Verbindungen soll com_remote.cxx noch funktionieren. */
  private def checkTasks() {
    val eventPipe = controller.newEventPipe()
    for (i <- 1 to 2) {
      scheduler executeXml <start_job job={testJobPath.string}/>
      eventPipe.nextWithCondition[TaskEndedEvent] { _.jobPath == testJobPath }
    }
    eventPipe.close()
  }
}


private object JS1079IT {
  private val connectionsMax = 5000   // Bei 16359 (Windows 8.1, 12GB): java.net.SocketException: No buffer space available (maximum connections reached?): connect; Bei 8000 (Unbuntu 12.04, 8GB): ERRNO-24 Too many open files
  private val testJobPath = JobPath.of("/test")

  private def schedulerXml(tcpPort: Int) =
    <spooler>
      <config port={tcpPort.toString}>
        <security>
          <allowed_host host="127.0.0.1" level="all"/>
        </security>
      </config>
    </spooler>
}

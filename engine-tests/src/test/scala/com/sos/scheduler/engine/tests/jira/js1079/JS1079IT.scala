package com.sos.scheduler.engine.tests.jira.js1079

import com.google.common.base.Charsets.UTF_8
import com.google.common.io.Files
import com.sos.jobscheduler.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1079.JS1079IT._
import java.io.File
import java.net._
import org.junit.runner.RunWith
import org.scalatest.FunSuite
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
final class JS1079IT extends FunSuite with ScalaSchedulerTest {

  private val nextUdpSocket = new UdpSocketGenerator
  private lazy val allDirectory = new File(controller.environment.configDirectory, "remote/_all")
  private lazy val serverTcpPort = findRandomFreeTcpPort()

  override def onBeforeSchedulerActivation(): Unit = {
    val schedulerXmlString = schedulerXml(serverTcpPort).toString()
    controller.prepare()
    Files.write(schedulerXmlString, new File(controller.environment.configDirectory, "scheduler.xml"), UTF_8)
    allDirectory.mkdirs()
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
    for (c <- clients) c expectUdp <check_folders/>
    for (c <- clients) c.close()
  }

  /** Auch bei den vielen Verbindungen soll com_remote.cxx noch funktionieren. */
  private def checkTasks(): Unit = {
    val eventPipe = controller.newEventPipe()
    for (i <- 1 to 2)
      runJob(testJobPath)
    eventPipe.close()
  }
}


private object JS1079IT {
  // Bei 16359 (Windows 8.1, 12GB): java.net.SocketException: No buffer space available (maximum connections reached?): connect
  // Bei 3953 (Windows 2003 server, 2GB): java.net.SocketException: No buffer space available (maximum connections reached?): connect
  // Bei 8000 (Unbuntu 12.04, 8GB): ERRNO-24 Too many open files
  private val connectionsMax = 3500
  private val testJobPath = JobPath("/test")

  private def schedulerXml(tcpPort: Int) =
    <spooler>
      <config port={tcpPort.toString}>
        <security>
          <allowed_host host="127.0.0.1" level="all"/>
        </security>
      </config>
    </spooler>
}

package com.sos.scheduler.engine.tests.jira.js795

import java.net.URI
import org.apache.log4j.Logger
import org.hamcrest.MatcherAssert.assertThat
import org.hamcrest.core.StringContains.containsString
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sun.jersey.api.client.Client
import org.scalatest.BeforeAndAfterAll
import javax.ws.rs.core.MediaType

/**JS-795: Einbau von Jetty in den JobScheduler. */
final class JS795aTest extends ScalaSchedulerTest with BeforeAndAfterAll {
  import JS795aTest._

  private lazy val server = new MyJettyServer(jettyPortNumber, contextPath, scheduler.getGuiceModule)
  private val client = Client.create()

  override def beforeAll() {
    controller.activateScheduler("-port=" + cppPortNumber)
    var ok = false
    try {
      server.start()
      ok = true
    }
    finally if (!ok) afterAll()
  }

  val commandResource = client.resource(contextUri +"/command")


  test("Execute a command via POST ") {
    val result = commandResource.accept(MediaType.TEXT_XML_TYPE).`type`(MediaType.TEXT_XML_TYPE)
      .post(classOf[String], "<show_state/>");
    checkCommandResult(result)
  }

  test("Execute a command via GET ") {
    val result = commandResource.queryParam("command", "<show_state/>").accept(MediaType.TEXT_XML_TYPE)
      .get(classOf[String])
    checkCommandResult(result)
  }

  private def checkCommandResult(result: String) {
    assertThat(result, containsString("<state"))
    assertThat(result, containsString(CommandServlet.testString))
  }

  override def afterAll() {
    server.stop()
    controller.terminateScheduler()
    server.close()
    super.afterAll()
  }
}

object JS795aTest {
  private val logger: Logger = Logger.getLogger(classOf[JS795aTest])
  private val cppPortNumber = 44441
  private val jettyPortNumber = 44440
  private val contextPath = "/JobScheduler"
  private val contextUri = new URI("http://localhost:"+ jettyPortNumber + contextPath)

  // TODO URIs und REST
  // Alle REST-Aufrufe liefern XML oder JSON.
  // "/objects/" liefert Objekte: Ordner, Jobs usw., nicht verschachtelt.
  // "/objects//PATH/" liefert Inhalt des Pfads
  // "/objects//JOBPATH.job/log&snapshot=true"  snapshot liefert nur den aktuellen Stand, sonst fortlaufend bis Log beendet ist.
  // "/objects//JOBPATH.job/description"
  // "/objects//JOBPATH.task/TASKID/log"
  // "/objects//JOBCHAINPATH.jobChain/orders/ORDERID/log"
  // "/objects//JOBCHAINPATH.jobChain/orders/ORDERID/log&historyId=.."
  // "/log" Hauptprotokoll
  // "/configuration.xml"
  // "/command&command=XMLCOMMAND"
  // "/command"  POST
  // "/static/" ?
  // "/z/" ?
  // "/" liefert <show_state what="all,orders"/>

  // TODO Logs
  // TODO Fortlaufende Logs
  // TODO XML oder JSON
  // TODO Authentifizierung
  // TODO HTTPS
  // TODO Request-Log
  // TODO WAR-Files
  // TODO GZIP
  // TODO Massentests: Viele Anfragen gleichzeitig. Anzahl der Threads soll klein bleiben.
}

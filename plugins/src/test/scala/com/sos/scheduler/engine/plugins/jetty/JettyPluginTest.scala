package com.sos.scheduler.engine.plugins.jetty

import org.hamcrest.MatcherAssert.assertThat
import org.hamcrest.core.StringContains.containsString
import com.sun.jersey.api.client.Client
import javax.ws.rs.core.MediaType._
import com.sos.scheduler.engine.test.scala.{CheckedBeforeAll, ScalaSchedulerTest}
import org.apache.log4j.Logger
import java.net.URI
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner

/**JS-795: Einbau von Jetty in den JobScheduler. */
@RunWith(classOf[JUnitRunner])
final class JettyPluginTest extends ScalaSchedulerTest with CheckedBeforeAll {
  import JettyPluginTest._

  private val client = Client.create()

  override protected def checkedBeforeAll(configMap: Map[String, Any]) {
    controller.activateScheduler("-e")
    super.checkedBeforeAll(configMap)
  }

  val commandResource = client.resource(contextUri+"/command")

  test("Execute a command via POST ") {
    val result = commandResource.accept(TEXT_XML_TYPE).`type`(TEXT_XML_TYPE).post(classOf[String], "<show_state/>");
    checkCommandResult(result)
  }

  test("Execute a command via GET ") {
    val result = commandResource.queryParam("command", "<show_state/>").accept(TEXT_XML_TYPE).get(classOf[String])
    checkCommandResult(result)
  }

  private def checkCommandResult(result: String) {
    assertThat(result, containsString("<state"))
  }

  override def afterAll() {
    try controller.terminateScheduler()
    finally super.afterAll()
  }
}

object JettyPluginTest {
  private val logger: Logger = Logger.getLogger(classOf[JettyPlugin])
  private val jettyPortNumber = 44440
  private val contextPath = "/JobScheduler"
  private val contextUri = new URI("http://localhost:"+ jettyPortNumber + contextPath)

  // TODO URIs und REST
  // Alle REST-Aufrufe liefern XML oder JSON.
  // "/jobs/PATH" liefert Jobs
  // "/folders/PATH" liefert Ordner
  // ...
  // "/jobs//PATH/log&snapshot=true"  snapshot liefert nur den aktuellen Stand, sonst fortlaufend bis Log beendet ist.
  // "/jobs//PATH/description"
  // "/jobs//PATH/task/TASKID/log"
  // "/job_chains//PATH/orders/ORDERID/log"
  // "/job_chains//PATH/orders/ORDERID/log&historyId=.."
  // ODER
  // "/objects/" liefert Objekte: Ordner, Jobs usw., nicht verschachtelt.
  // "/objects//PATH/" liefert Inhalt des Pfads
  // "/objects//JOBPATH.job/log&snapshot=true"  snapshot liefert nur den aktuellen Stand, sonst fortlaufend bis Log beendet ist.
  // "/objects//JOBPATH.job/description"
  // "/objects//JOBPATH.task/TASKID/log"
  // "/objects//JOBCHAINPATH.jobChain/orders/ORDERID/log"
  // "/objects//JOBCHAINPATH.jobChain/orders/ORDERID/log&historyId=.."
  //
  // "/log" Hauptprotokoll
  // "/configuration.xml"
  // "/static/" ?
  // "/z/" ?
  // "/" liefert <show_state what="all,orders"/>

  // ERLEDIGT:
  // "/command&command=XMLCOMMAND"
  // "/command"  POST

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

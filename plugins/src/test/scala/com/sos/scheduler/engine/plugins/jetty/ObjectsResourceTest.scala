package com.sos.scheduler.engine.plugins.jetty

import java.net.URI
import javax.ws.rs.core.MediaType._
import com.sos.scheduler.engine.test.scala.{CheckedBeforeAll, ScalaSchedulerTest}
import com.sun.jersey.api.client.Client
import org.apache.log4j.Logger
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

/** JS-795: Einbau von Jetty in den JobScheduler. */
@RunWith(classOf[JUnitRunner])
final class ObjectsResourceTest extends ScalaSchedulerTest with CheckedBeforeAll {
  import ObjectsResourceTest._

  private val client = Client.create()

  override protected def checkedBeforeAll(configMap: Map[String, Any]) {
    controller.activateScheduler()
    super.checkedBeforeAll(configMap)
  }

  private val objectsResource = client.resource(contextUri+"/objects")

  test("Read a log") {
    val result = objectsResource.path("a.job/log").accept(TEXT_PLAIN_TYPE).get(classOf[String])
    result should include ("SCHEDULER-893")  // SCHEDULER-893  Job is 'active' now
    logger.info(result)
  }
}

object ObjectsResourceTest {
  private val logger: Logger = Logger.getLogger(classOf[ObjectsResourceTest])
  private val jettyPortNumber = 44440
  private val contextUri = new URI("http://localhost:"+ jettyPortNumber + JettyPlugin.contextPath)
}

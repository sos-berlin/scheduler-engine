package com.sos.scheduler.engine.plugins.webservice.services

import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginJerseyTester
import com.sos.scheduler.engine.plugins.webservice.tests.Tests
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import javax.ws.rs.core.MediaType._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import spray.json._

@RunWith(classOf[JUnitRunner])
final class FolderServiceIT extends FreeSpec with ScalaSchedulerTest with JettyPluginJerseyTester {

  override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    testPackage = Some(Tests.testPackage))

  "Read job list as JSON" in {
    get[String]("/jobscheduler/engine/folder?type=job&folder=/", Accept = List(APPLICATION_JSON_TYPE)).parseJson shouldEqual
      s"""{
        "entries": [
          {
            "name": "a",
            "uri":"http://$host:$port/jobscheduler/engine/job?job=/a"
          }
        ],
        "typeName": "job",
        "folderPath": "/"
      }""".parseJson
  }
}

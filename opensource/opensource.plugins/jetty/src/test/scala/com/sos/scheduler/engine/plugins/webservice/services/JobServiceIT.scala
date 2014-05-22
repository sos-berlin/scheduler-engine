package com.sos.scheduler.engine.plugins.webservice.services

import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.plugins.jetty.test.{JettyPluginJerseyTester, JettyPluginTests}
import JettyPluginTests.aJobPath
import com.sos.scheduler.engine.plugins.webservice.tests.Tests
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import javax.ws.rs.core.MediaType._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JobServiceIT extends FreeSpec with ScalaSchedulerTest with JettyPluginJerseyTester {

  override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    testPackage = Some(Tests.testPackage))

  "Read a job configuration" in {
    val jobXml = instance[JobSubsystem].job(aJobPath).sourceXmlBytes
    get[Array[Byte]]("/jobscheduler/engine/job/configuration?job=/a", Accept = List(TEXT_XML_TYPE)) shouldEqual jobXml
  }

  "Read a job description" in {
    get[String]("/jobscheduler/engine/job/description?job=/a", Accept = List(TEXT_PLAIN_TYPE)) shouldEqual "TEST-DESCRIPTION mit Ümläüten"
  }

  "Read a job log" in {
    get[String]("/jobscheduler/engine/job/log?job=/a", Accept = List(TEXT_PLAIN_TYPE)) should include ("SCHEDULER-893")
  }
}

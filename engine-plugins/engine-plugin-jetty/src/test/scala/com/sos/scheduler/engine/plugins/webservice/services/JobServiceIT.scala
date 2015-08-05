package com.sos.scheduler.engine.plugins.webservice.services

import com.sos.scheduler.engine.common.system.OperatingSystem._
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginTests.{AJobPath, UmlautJobPath}
import com.sos.scheduler.engine.plugins.jetty.test.{JettyPluginJerseyTester, ProvideUmlautJob}
import com.sos.scheduler.engine.plugins.webservice.tests.Tests
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import javax.ws.rs.core.MediaType._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JobServiceIT extends FreeSpec with ScalaSchedulerTest with JettyPluginJerseyTester with ProvideUmlautJob {

  override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    testPackage = Some(Tests.testPackage))

  "Read a job configuration" in {
    val jobXml = instance[JobSubsystem].job(AJobPath).sourceXmlBytes
    get[Array[Byte]]("/jobscheduler/engine/job/configuration?job=/a", Accept = List(TEXT_XML_TYPE)) shouldEqual jobXml
  }

//  "Read a job configuration, job named with umlauts, Windows only" in {
//    if (isUnix) pending  // Unix encodes filenames with UTF-8 but JobScheduler decodes then with ISO-8859-1 (see JS-1374)
//    pending // Jenkins fails due to invalid encoded umlauts in test output: Failed to read test report file ...\TEST-com.sos.scheduler.engine.plugins.webservice.services.EventsServiceIT.xml
//            // org.dom4j.DocumentException: Invalid byte 2 of 3-byte UTF-8 sequence. Nested exception: Invalid byte 2 of 3-byte UTF-8 sequence.
//    provideUmlautJob()
//    val jobXml = instance[JobSubsystem].job(UmlautJobPath).sourceXmlBytes
//    get[Array[Byte]]("/jobscheduler/engine/job/configuration?job=/test-umlauts-äöüßÄÖÜ", Accept = List(TEXT_XML_TYPE)) shouldEqual jobXml
//  }

  "Read a job description" in {
    get[String]("/jobscheduler/engine/job/description?job=/a", Accept = List(TEXT_PLAIN_TYPE)) shouldEqual "TEST-DESCRIPTION mit Ümläüten"
  }

  "Read a job log" in {
    get[String]("/jobscheduler/engine/job/log?job=/a", Accept = List(TEXT_PLAIN_TYPE)) should include ("SCHEDULER-893")
  }
}

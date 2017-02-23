package com.sos.scheduler.engine.tests.jira.js1093

import com.sos.jobscheduler.common.system.OperatingSystem.isUnix
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS1093IT extends FunSuite with ScalaSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    mainArguments = List("-distributed-orders"))

  if (!isUnix) {    // Hostware kommt bei Locale en_DE nicht mit H2Database-Zeitstempel klar (endet mit ",000", siehe sosdb.cxx)
    test("JS-1093 JobScheduler crashes when a <modify_order.../> command is called for a suspended order in a distributed job chain") {
      scheduler executeXml <order job_chain="/test" id="1"><run_time/></order>
      scheduler executeXml <modify_order job_chain="/test" order="1" suspended="true"/>
      scheduler executeXml <modify_order job_chain="/test" order="1" suspended="false"/>
    }
  }
}

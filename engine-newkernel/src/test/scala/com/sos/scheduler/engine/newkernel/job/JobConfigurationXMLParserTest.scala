package com.sos.scheduler.engine.newkernel.job

import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.common.time.Stopwatch
import com.sos.scheduler.engine.newkernel.job.JobConfigurationXMLParserTest._
import org.joda.time.DateTimeZone
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner


@RunWith(classOf[JUnitRunner])
final class JobConfigurationXMLParserTest extends FunSuite {

  test("parse") {
    val xml =
      <new_job title="TITLE">
        <run_time begin="09:00" end="17:00" repeat="5"/>
        <script language="shell">exit 0</script>
      </new_job>
      .toString()
    val jobConfiguration = JobConfigurationXMLParser.parseString(xml, DateTimeZone.getDefault)
    assert(jobConfiguration.title == "TITLE")
    assert(jobConfiguration.script.asInstanceOf[ShellScript].text == "exit 0")
  }

  val n = 100000
  test(s"$n x JobConfigurationXMLParser") {
    val xml =
      <new_job title="TITLE">
        <run_time begin="09:00" end="17:00" repeat="5"/>
        <script language="shell">exit 0</script>
      </new_job>
      .toString()
    val stopwatch = new Stopwatch()
    for (i <- 1 to n) JobConfigurationXMLParser.parseString(xml, DateTimeZone.getDefault)
    logger.debug(s"${n*1000 / stopwatch.elapsedMs} XMLs/s")
  }
}

private object JobConfigurationXMLParserTest {
  private val logger = Logger(getClass)
}

/*
        <schedule>
          <daily weekdays="monday tuesday sunday" monthdays="1 2 3 -1" dates="2013-04-01 2013-05-01">
            <period begin="09:00" end="17:00" repeat_after="5"/>
          </daily>
          <daily months="april may" monthdays="1 2 3 -1">
            <period begin="09:00" end="17:00" every="5s"/>
          </daily>
          <at instant="2013-04-01 12:00"/>
        </schedule>
*/

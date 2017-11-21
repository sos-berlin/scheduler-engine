package com.sos.scheduler.engine.tests.jira.js1749

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.kernel.Scheduler.DefaultZoneId
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1749.JS1749IT._
import java.time.LocalDateTime
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1749IT extends FreeSpec with ScalaSchedulerTest {

  "Multiple <date> for the same day are merged" in {
    scheduler.executeXml(
      <order job_chain="/test" id="TEST-ORDER">
        <run_time>
          <date date="2030-01-02">
            <period single_start="01:00"/>
          </date>
          <date date="2030-01-02">
            <period single_start="02:00"/>
          </date>
          <date date="2030-01-03">
            <period single_start="03:00"/>
          </date>
        </run_time>
      </order>)
    val response = scheduler executeXml <show_calendar what="orders" before="2030-02-01T00:00:00Z"/>
    assert((response.answer \ "calendar").head == xml.Utility.trim(
      <calendar>
        <at order="TEST-ORDER" job_chain="/test" at={toUtc("2030-01-02T01:00:00")}/>
        <period single_start={toUtc("2030-01-02T01:00:00")} order="TEST-ORDER" job_chain="/test"/>
        <period single_start={toUtc("2030-01-02T02:00:00")} order="TEST-ORDER" job_chain="/test"/>
        <period single_start={toUtc("2030-01-03T03:00:00")} order="TEST-ORDER" job_chain="/test"/>
      </calendar>))
  }

  //"Different days starting with an elapsed time" in {
  //  // Test works only on 2017-11-21
  //  scheduler.executeXml(
  //    <order job_chain="/test" id="TEST-ORDER">
  //      <run_time>
  //        <date date="2017-11-21">
  //          <period single_start="10:00"/>
  //        </date>
  //        <date date="2017-11-22">
  //          <period single_start="10:00"/>
  //        </date>
  //        <date date="2017-11-23">
  //          <period single_start="10:00"/>
  //        </date>
  //        <date date="2017-11-24">
  //          <period single_start="10:00"/>
  //        </date>
  //        <date date="2018-01-24">
  //          <period single_start="10:00"/>
  //        </date>
  //        <holidays/>
  //      </run_time>
  //  </order>)
  //  val response = scheduler executeXml <show_calendar from='2017-11-20T00:00:00' before='2018-11-28T00:00:00' what='orders'/>
  //  logger.info(response.string)
  //  assert((response.answer \ "calendar").head == xml.Utility.trim(
  //    <calendar>
  //      <at order="TEST-ORDER" job_chain="/test" at="2017-11-22T09:00:00Z"/>
  //      <period single_start="2017-11-21T09:00:00Z" order="TEST-ORDER" job_chain="/test"/>
  //      <period single_start="2017-11-22T09:00:00Z" order="TEST-ORDER" job_chain="/test"/>
  //      <period single_start="2017-11-23T09:00:00Z" order="TEST-ORDER" job_chain="/test"/>
  //      <period single_start="2017-11-24T09:00:00Z" order="TEST-ORDER" job_chain="/test"/>
  //      <period single_start="2018-01-24T09:00:00Z" order="TEST-ORDER" job_chain="/test"/>
  //    </calendar>))
  //}
}

private object JS1749IT {
  private val logger = Logger(getClass)

  private def toUtc(localDateTime: String) =
    LocalDateTime.parse(localDateTime).toInstant(DefaultZoneId).toString
}

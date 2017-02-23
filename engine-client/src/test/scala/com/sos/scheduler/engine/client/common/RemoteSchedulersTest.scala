package com.sos.scheduler.engine.client.common

import com.sos.jobscheduler.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.jobscheduler.common.scalautil.xmls.XmlSources.stringToSource
import com.sos.scheduler.engine.client.common.RemoteSchedulers._
import javax.xml.stream.events.StartElement
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class RemoteSchedulersTest extends FreeSpec {

  "readSchedulerResponses" in {
    val xmlString = <spooler><answer time="2014-10-07T10:43:07.191Z"><aaa/><bbb/></answer></spooler>.toString()
    readSchedulerResponses(xmlString)(read) shouldEqual List("aaa", "bbb")
  }

  "readSchedulerResponses with ERROR" in {
    intercept[RuntimeException] {
      val xmlString = <spooler><answer time="2014-10-07T10:43:07.191Z"><ERROR text="MESSAGE"/></answer></spooler>.toString()
      readSchedulerResponses(xmlString)(read)
    } .getMessage shouldEqual "MESSAGE"
  }

  "readSchedulerResponse with one element" in {
    val xmlString = <spooler><answer time="2014-10-07T10:43:07.191Z"><aaa/></answer></spooler>.toString()
    readSchedulerResponse(xmlString)(read) shouldEqual "aaa"
  }

  "readSchedulerResponse with missing element" in {
    val xmlString = <spooler><answer time="2014-10-07T10:43:07.191Z"></answer></spooler>.toString()
    intercept[RuntimeException] {
      readSchedulerResponse(xmlString)(read)
    }
    .getMessage should include ("Incomplete XML response")
  }

  "readSchedulerResponse with too much elements" in {
    val xmlString = <spooler><answer time="2014-10-07T10:43:07.191Z"><aaa/><bbb/></answer></spooler>.toString()
    intercept[RuntimeException] {
      readSchedulerResponse(xmlString)(read)
    }
    .getMessage should include ("More than one")
  }

  private def read(eventReader: ScalaXMLEventReader): String = {
    import eventReader._
    parseStartElement() {
      eat[StartElement].getName.getLocalPart
    }
  }
}

package com.sos.scheduler.engine.client.common

import com.sos.scheduler.engine.client.common.RemoteSchedulers._
import com.sos.scheduler.engine.common.scalautil.xmls.{ScalaXMLEventReader, StringSource}
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

  "checkForError without error" in {
    val xmlString = <spooler><answer time="2014-10-07T10:43:07.191Z"><aaa/><bbb/></answer></spooler>.toString()
    checkResponseForError(StringSource(xmlString))
  }

  "checkForError" in {
    val xmlString = <spooler><answer time="2014-10-07T10:43:07.191Z"><ERROR text="TEST"/></answer></spooler>.toString()
    intercept[XmlResponseException] { checkResponseForError(StringSource(xmlString)) } .getMessage should include ("TEST")
  }

  "readSchedulerResponses" in {
    val xmlString = <spooler><answer time="2014-10-07T10:43:07.191Z"><aaa/><bbb/></answer></spooler>.toString()
    readSchedulerResponses(StringSource(xmlString))(read) shouldEqual List("aaa", "bbb")
  }

  "readSchedulerResponses with ERROR" in {
    intercept[RuntimeException] {
      val xmlString = <spooler><answer time="2014-10-07T10:43:07.191Z"><ERROR text="MESSAGE"/></answer></spooler>.toString()
      readSchedulerResponses(StringSource(xmlString))(read)
    } .getMessage shouldEqual "MESSAGE"
  }

  "readSchedulerResponse with one element" in {
    val xmlString = <spooler><answer time="2014-10-07T10:43:07.191Z"><aaa/></answer></spooler>.toString()
    readSchedulerResponse(StringSource(xmlString))(read) shouldEqual "aaa"
  }

  "readSchedulerResponse with missing element" in {
    val xmlString = <spooler><answer time="2014-10-07T10:43:07.191Z"></answer></spooler>.toString()
    intercept[RuntimeException] {
      readSchedulerResponse(StringSource(xmlString))(read)
    }
    .getMessage should include ("Incomplete XML response")
  }

  "readSchedulerResponse with too much elements" in {
    val xmlString = <spooler><answer time="2014-10-07T10:43:07.191Z"><aaa/><bbb/></answer></spooler>.toString()
    intercept[RuntimeException] {
      readSchedulerResponse(StringSource(xmlString))(read)
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

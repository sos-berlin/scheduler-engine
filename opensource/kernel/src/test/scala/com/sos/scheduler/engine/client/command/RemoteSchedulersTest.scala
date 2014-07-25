package com.sos.scheduler.engine.client.command

import com.sos.scheduler.engine.client.command.RemoteSchedulers._
import com.sos.scheduler.engine.common.scalautil.xml.{ScalaXMLEventReader, StringSource}
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
    val xmlString = <spooler><answer><aaa/><bbb/></answer></spooler>.toString()
    readSchedulerResponses(StringSource(xmlString))(read) shouldEqual List("aaa", "bbb")
  }

  "readSchedulerResponses with ERROR" in {
    intercept[RuntimeException] {
      val xmlString = <spooler><answer><ERROR text="MESSAGE"/></answer></spooler>.toString()
      readSchedulerResponses(StringSource(xmlString))(read)
    } .getMessage shouldEqual "MESSAGE"
  }

  "readSchedulerResponse with one element" in {
    val xmlString = <spooler><answer><aaa/></answer></spooler>.toString()
    readSchedulerResponse(StringSource(xmlString))(read) shouldEqual "aaa"
  }

  "readSchedulerResponse with missing element" in {
    val xmlString = <spooler><answer></answer></spooler>.toString()
    intercept[RuntimeException] {
      readSchedulerResponse(StringSource(xmlString))(read)
    }
  }

  "readSchedulerResponse with too much elements" in {
    val xmlString = <spooler><answer><aaa/><bbb/></answer></spooler>.toString()
    intercept[RuntimeException] {
      readSchedulerResponse(StringSource(xmlString))(read)
    }
  }

  private def read(eventReader: ScalaXMLEventReader): String = {
    import eventReader._
    parseElement() {
      val result = peek.asStartElement.getName.getLocalPart
      ignoreAttributes()
      result
    }
  }
}

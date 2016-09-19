package com.sos.scheduler.engine.client.web

import com.sos.scheduler.engine.client.web.WebCommandClient.{XmlException, checkResponseForError}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class WebCommandClientTest extends FreeSpec {

  "checkResponseForError without error" in {
    val xmlString = <spooler><answer time="2014-10-07T10:43:07.191Z"><aaa/><bbb/></answer></spooler>.toString()
    checkResponseForError(xmlString)
  }

  "checkResponseForError" in {
    val xmlString = <spooler><answer time="2014-10-07T10:43:07.191Z"><ERROR text="TEST"/></answer></spooler>.toString()
    intercept[XmlException] { checkResponseForError(xmlString) } .getMessage should include ("TEST")
  }
}

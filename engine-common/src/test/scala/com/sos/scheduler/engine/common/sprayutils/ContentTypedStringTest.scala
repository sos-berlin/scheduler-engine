package com.sos.scheduler.engine.common.sprayutils

import java.nio.charset.StandardCharsets.UTF_8
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import spray.http.HttpCharsets.`UTF-8`
import spray.http.HttpEntity
import spray.http.MediaTypes._
import spray.httpx.marshalling._
import spray.testkit.ScalatestRouteTest

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class ContentTypedStringTest extends FreeSpec with ScalatestRouteTest {

  private val testString = "<p>Äa</p>"
  private val htmlString = ContentTypedString(`text/html` withCharset `UTF-8`)(testString)

  "Marshal as text/html" in {
    val entity = HttpEntity(`text/html` withCharset `UTF-8`, testString.getBytes(UTF_8))
    assert(marshal(htmlString) == Right(entity))
  }
}

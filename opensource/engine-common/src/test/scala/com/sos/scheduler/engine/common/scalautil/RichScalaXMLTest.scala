package com.sos.scheduler.engine.common.scalautil

import org.scalatest.FunSuite
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import RichScalaXML.RichElem
import org.scalatest.matchers.ShouldMatchers._

@RunWith(classOf[JUnitRunner])
class RichScalaXMLTest extends FunSuite {

  test("attributeText") {
    val e = <a b="B"/>
    e.attributeText("b") should equal ("B")
    e.attributeText("x") should equal ("")
  }
}

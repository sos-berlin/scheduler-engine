package com.sos.scheduler.engine.common.xml

import com.sos.scheduler.engine.common.xml.VariableSets.{parseXml, toParamsXmlElem, toXmlElem}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class VariableSetsTest extends FreeSpec {

  "parseXml" in {
    val x =
      <anything>
        <variable name="A" value="a"/>
        <variable name="B" hex_value="620180"/>
      </anything>
    assert(parseXml(x.toString()) == Map("A" → "a", "B" → "b\u0001\u0080"))
  }

  "toXmlElem" in {
    assert(toXmlElem(Map("A" → "a")) == <sos.spooler.variable_set><variable name="A" value="a"/></sos.spooler.variable_set>)
    assert(toXmlElem(Map()) == <sos.spooler.variable_set/>)
    assert(toXmlElem(Map("A" → "a"), "params", "param") == <params><param name="A" value="a"/></params>)
  }

  "toXmlElem o parseXml" in {
    val m = Map("A" → "AA", "b" → "b\u0001\u0080")
    val x = toXmlElem(m)
    assert(x == <sos.spooler.variable_set><variable name="A" value="AA"/><variable name="b" hex_value="620180"/></sos.spooler.variable_set>)
    assert(parseXml(x.toString()) == m)
  }

  "toParamsXmlElem" in {
    assert(toParamsXmlElem(Map("A" → "a")) == <params><param name="A" value="a"/></params>)
    assert(toParamsXmlElem(Map()) == <params/>)
  }

  "Invalid hex_value is rejected" in {
    parseXml(<anything><variable name="B" hex_value=""/></anything>.toString())
    intercept[RuntimeException] {
      parseXml(<anything><variable name="B" hex_value="1g"/></anything>.toString())
    }
    intercept[RuntimeException] {
      parseXml(<anything><variable name="B" hex_value="1"/></anything>.toString())
    }
  }
}

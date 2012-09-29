package com.sos.scheduler.engine.data.test

import org.codehaus.jackson.map.ObjectMapper
import org.junit.Test
import org.scalatest.FunSuite
import com.fasterxml.jackson.module.scala.DefaultScalaModule

/** Zum Test von JSON-Serialisierungen.
  * @author Joacim Zschimmer */
trait JsonTest {
  this: FunSuite =>

  private lazy val defaultObjectMapper = {
    val o = new ObjectMapper
    o.registerModule(DefaultScalaModule)
    o
  }

  def addJsonTests[A <: AnyRef](obj: A, json: String) {
    addJsonTests(defaultObjectMapper, obj, json)
  }

  def addJsonTests[A <: AnyRef](objectMapper: ObjectMapper, obj: A, json: String) {
    val serializerTester = new SerializerTester(objectMapper)

    test("Serialize to JSON") {
      serializerTester.assertObjectIsSerializedTo(obj, json)
    }

    test("Deserialize from JSON") {
      serializerTester.assertJsonIsDeserializedTo(json, obj)
    }
  }
}

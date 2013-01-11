package com.sos.scheduler.engine.data.test

import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.module.scala.DefaultScalaModule
import org.scalatest.FunSuite

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

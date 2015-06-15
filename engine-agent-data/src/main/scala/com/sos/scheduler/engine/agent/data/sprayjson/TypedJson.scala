package com.sos.scheduler.engine.agent.data.sprayjson

import spray.json.{JsObject, JsString, JsValue}

/**
 * @author Joacim Zschimmer
 */

object TypedJson {
  implicit class WithFieldsJsObject(val delegate: JsObject) extends AnyVal {
    def withTypeField(typeName: String) = JsObject(delegate.fields + ("$TYPE" → JsString(typeName)))
  }

  def splitTypeAndJsObject(value: JsValue): (String, JsObject) =
    value.asJsObject.fields("$TYPE").asInstanceOf[JsString].value → JsObject(value.asJsObject.fields - "$TYPE")
}

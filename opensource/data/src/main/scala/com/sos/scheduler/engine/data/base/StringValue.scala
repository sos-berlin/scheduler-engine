package com.sos.scheduler.engine.data.base

abstract class StringValue(val string: String) extends IsString {
  /** @deprecated */
  final def asString = string
}

object StringValue {
  /** Für &lt;elememt attribute={stringValue}/>. */
  implicit def toXmlText(o: StringValue) = new xml.Text(o.string)
}
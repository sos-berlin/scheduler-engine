package com.sos.scheduler.engine.data.base

import org.codehaus.jackson.map.annotate.JsonSerialize

@JsonSerialize(using = classOf[StringValueSerializer])
abstract class StringValue(val string: String) {
  final def asString = string

  final def isEmpty = string.isEmpty

  override def equals(o: Any) = o match {
    case o: StringValue => (getClass eq o.getClass) && string == o.string
    case _ => false
  }

  final override def hashCode = string.hashCode

  override def toString = string
}

object StringValue {
  /** Für &lt;elememt attribute={stringValue}/>. */
  implicit def toXmlText(o: StringValue) = new xml.Text(o.string)
}
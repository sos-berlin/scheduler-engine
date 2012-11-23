package com.sos.scheduler.engine.data.base

import org.codehaus.jackson.map.annotate.JsonSerialize
import javax.annotation.Nullable

@JsonSerialize(using = classOf[IsStringSerializer])
trait IsString {
  val string: String

  final def isEmpty = string.isEmpty

  override def equals(o: Any) = o match {
    case o: IsString => (getClass eq o.getClass) && string == o.string
    case _ => false
  }

  final override def hashCode = string.hashCode

  override def toString = string
}

object IsString {
  /** Für &lt;elememt attribute={stringValue}/>. */
  implicit def toXmlText(o: StringValue) = new xml.Text(o.string)

  @Nullable def stringOrNull[A <: IsString](o: Option[A]): String = o match {
    case Some(a) => a.string
    case None => null
  }
}

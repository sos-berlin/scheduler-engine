package com.sos.scheduler.engine.newkernel.utils

import javax.xml.stream.events.{Attribute, StartElement}
import scala.collection.JavaConversions._

object ScalaStax {

  implicit final class RichStartElement(val delegate: StartElement) extends AnyVal {
    def attributes: Iterator[Attribute] =
      delegate.getAttributes.asInstanceOf[java.util.Iterator[Attribute]]
  }
}

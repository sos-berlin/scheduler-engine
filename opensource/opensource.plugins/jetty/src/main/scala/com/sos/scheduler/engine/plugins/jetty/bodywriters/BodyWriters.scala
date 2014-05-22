package com.sos.scheduler.engine.plugins.jetty.bodywriters

import javax.ws.rs.ext.MessageBodyWriter

object BodyWriters {
  val messageBodyWriters = Iterable[Class[_ <: MessageBodyWriter[_]]](
    classOf[XmlElemWriter])
}

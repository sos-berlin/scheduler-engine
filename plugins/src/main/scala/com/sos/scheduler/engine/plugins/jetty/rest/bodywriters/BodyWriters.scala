package com.sos.scheduler.engine.plugins.jetty.rest.bodywriters

import javax.ws.rs.ext.MessageBodyWriter

object BodyWriters {
  val messageBodyWriters: Iterable[Class[_ <: MessageBodyWriter[_]]] = Iterable(
    classOf[XmlElemWriter])
}

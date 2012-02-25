package com.sos.scheduler.engine.plugins.jetty.rest.marshal

import org.codehaus.jackson.JsonGenerator

trait Marshaller[A] {
  val objectClass: Class[A]

  def marshal(j: JsonGenerator, a: A)
}

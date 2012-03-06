package com.sos.scheduler.engine.plugins.jetty.rest.marshal

import org.codehaus.jackson.JsonFactory
import java.io.OutputStream

class MarshallerRegister(marshallers: Iterable[Marshaller[_]]) {
  private val marshallerMap = (marshallers map { m => m.objectClass -> m: (Class[_], Marshaller[_]) }).toMap

  private lazy val jsonFactory = new JsonFactory

  def marshaller[A](c: Class[A]) = marshallerOption(c) getOrElse { throw new IllegalArgumentException("No marshaller for "+c) }

  def marshallerOption[A](c: Class[A]) = marshallerMap.get(c).asInstanceOf[Option[Marshaller[A]]]

  def marshalToJson[A <: AnyRef](a: A, out: OutputStream) {
    val j = jsonFactory.createJsonGenerator(out)
    marshaller(a.getClass.asInstanceOf[Class[A]]).marshal(j, a)
    j.close()
  }
}

object MarshallerRegister {
  def apply(m: Marshaller[_]*) = new MarshallerRegister(m)
}

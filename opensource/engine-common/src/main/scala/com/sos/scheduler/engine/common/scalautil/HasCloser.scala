package com.sos.scheduler.engine.common.scalautil

import HasCloser._
import com.google.common.io.Closer
import java.io.Closeable
import scala.language.reflectiveCalls

trait HasCloser extends AutoCloseable {

  private val _closer: Closer = Closer.create()

  implicit final def closer: Closer = {
    if (_closer == null) throw new NullPointerException(s"$getClass should extend HasClose further in front?")
    _closer
  }

  final def onClose(f: => Unit) {
    closer.register(toCloseable(f))
  }

  def close() {
    closer.close()
  }
}


object HasCloser {
  object implicits {
    implicit class RichCloser(val delegate: Closer) extends AnyVal {
      final def apply(f: => Unit) {
        delegate.register(toCloseable(f))
      }
    }

    implicit class RichClosable[A <: { def close() }](val delegate: A) extends AnyVal {
      final def registerCloseable(implicit closer: Closer): A = {
        closer.register(toCloseable { delegate.close() })
        delegate
      }
    }
  }

  private def toCloseable(f: => Unit) =
    new Closeable {
      def close() {
        f
      }
    }
}

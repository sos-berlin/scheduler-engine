package com.sos.scheduler.engine.common.scalautil

import com.google.common.io.Closer
import com.sos.scheduler.engine.common.scalautil.HasCloser._
import scala.language.reflectiveCalls

trait HasCloser extends AutoCloseable with CloseOnError {

  private val _closer: Closer = Closer.create()

  protected implicit final def closer: Closer = {
    if (_closer == null) throw new NullPointerException(s"$getClass should extend HasClose further in front?")
    _closer
  }

  protected final def whenNotClosedAtShutdown(body: ⇒ Unit) {
    val hook = new Thread(s"ShutdownHook for $toString") {
      override def run() {
        body
      }
    }
    Runtime.getRuntime.addShutdownHook(hook)
    onClose {
      Runtime.getRuntime.removeShutdownHook(hook)
    }
  }

  /** Registers the function for execution in close(), in reverse order of registering. */
  protected final def onClose(f: ⇒ Unit) {
    closer.register(toGuavaCloseable(f))
  }

  protected final def registerAutoCloseable(autoCloseable: AutoCloseable) {
    closer.register(toGuavaCloseable(autoCloseable))
  }

  def close() {
    closer.close()
  }
}


object HasCloser {
  private type GuavaCloseable = java.io.Closeable

  object implicits {
    implicit class RichCloser(val delegate: Closer) extends AnyVal {
      final def apply(f: ⇒ Unit) {
        delegate.register(toGuavaCloseable(f))
      }
    }

    implicit class RichClosable[A <: { def close() }](val delegate: A) extends AnyVal {
      final def registerCloseable(implicit closer: Closer): A = {
        closer.register(toGuavaCloseable { delegate.close() })
        delegate
      }
    }
  }

  private def toGuavaCloseable(autoCloseable: AutoCloseable): GuavaCloseable =
    autoCloseable match {
      case o: GuavaCloseable ⇒ o
      case o ⇒ toGuavaCloseable { o.close() }
    }

  private def toGuavaCloseable(f: ⇒ Unit): GuavaCloseable =
    new GuavaCloseable {
      def close() {
        f
      }
    }
}

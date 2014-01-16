package com.sos.scheduler.engine.common.scalautil

import com.google.common.io.Closer
import com.sos.scheduler.engine.common.utils.SosAutoCloseable
import java.io.Closeable

trait HasCloser extends SosAutoCloseable {

  val closer: Closer = Closer.create()

  final def onClose(f: => Unit) {
    closer.register(new Closeable {
      def close() {
        f
      }
    })
  }

  def close() {
    closer.close()
  }
}

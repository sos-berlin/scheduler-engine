package com.sos.scheduler.engine.newkernel.utils

import com.sos.scheduler.engine.common.utils.SosAutoCloseable

trait Service extends SosAutoCloseable {
  private var _started = false

  final def start() {
    if (_started) throw new IllegalStateException(s"${getClass.getSimpleName} can be started only once")
    onStart()
    _started = true
  }

  protected def onStart()

  final def stop() {
    onStop()
  }

  protected def onStop()
}

object Service {
  def withService[A](service: Service)(f: => A) {
    try {
      service.start()
      f
    } finally
      service.close()
  }
}


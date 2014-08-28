package com.sos.scheduler.engine.newkernel.utils

trait Service extends AutoCloseable {
  private var _started = false

  final def start(): Unit = {
    if (_started) throw new IllegalStateException(s"${getClass.getSimpleName} can be started only once")
    onStart()
    _started = true
  }

  protected def onStart()

  final def stop(): Unit = {
    onStop()
  }

  protected def onStop()
}

object Service {
  def withService[A](service: Service)(f: => A): Unit = {
    try {
      service.start()
      f
    } finally
      service.close()
  }
}


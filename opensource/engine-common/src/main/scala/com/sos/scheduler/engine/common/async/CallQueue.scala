package com.sos.scheduler.engine.common.async

import java.util.concurrent.Callable
import scala.sys._

trait CallQueue extends AutoCloseable {

  def close()

  def add(o: TimedCall[_])

  def tryCancel(o: TimedCall[_]): Boolean

  def nextTime: Long


  final def add[A](f: () => A) {
    add(ShortTermCall(f))
  }

  final def add(o: Runnable) {
    add(ShortTermCall(o))
  }

  final def add(o: Callable[_]) {
    add(ShortTermCall(o))
  }

  final def remove(o: TimedCall[_]) {
    val removed = tryCancel(o)
    if (!removed) error(s"Unknown TimedCall '$o'")
  }
}
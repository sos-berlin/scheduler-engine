package com.sos.scheduler.engine.common.async

import java.util.concurrent.Callable
import scala.sys._
import com.sos.scheduler.engine.common.utils.SosAutoCloseable

trait CallQueue extends SosAutoCloseable {
  def close()

  final def add[A](f: () => A) {
    add(ShortTermCall(f))
  }

  final def add(o: Runnable) {
    add(ShortTermCall(o))
  }

  final def add(o: Callable[_]) {
    add(ShortTermCall(o))
  }

  def add(o: TimedCall[_])

  final def remove(o: TimedCall[_]) {
    val removed = tryCancel(o)
    if (!removed) error(s"Unknown TimedCall '$o'")
  }

  def tryCancel(o: TimedCall[_]): Boolean

  def nextTime: Long
}
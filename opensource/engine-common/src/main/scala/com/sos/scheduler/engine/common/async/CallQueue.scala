package com.sos.scheduler.engine.common.async

import java.util.NoSuchElementException

trait CallQueue extends AutoCloseable {

  def apply(f: ⇒ Unit) {
    add(ShortTermCall { () ⇒ f })
  }

  def add[A](o: TimedCall[A])

  def tryCancel[A](o: TimedCall[A]): Boolean

  final def add(o: Runnable) {
    add(ShortTermCall(o))
  }

  final def remove(o: TimedCall[_]) {
    val removed = tryCancel(o)
    if (!removed) throw new NoSuchElementException(s"Unknown TimedCall '$o'")
  }

  def nextTime: Long
}

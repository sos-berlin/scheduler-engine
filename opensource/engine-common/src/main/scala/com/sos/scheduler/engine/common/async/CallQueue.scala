package com.sos.scheduler.engine.common.async

import java.util.NoSuchElementException
import org.joda.time.Instant

trait CallQueue extends AutoCloseable {

  def at[A](t: Instant)(f: ⇒ A): TimedCall[A] = {
    val call = TimedCall(t)(f)
    add(call)
    call
  }

  def apply(f: ⇒ Unit): Unit = {
    add(ShortTermCall { () ⇒ f })
  }

  def add[A](o: TimedCall[A])

  def tryCancel[A](o: TimedCall[A]): Boolean

  final def add(o: Runnable): Unit = {
    add(ShortTermCall(o))
  }

  final def remove(o: TimedCall[_]): Unit = {
    val removed = tryCancel(o)
    if (!removed) throw new NoSuchElementException(s"Unknown TimedCall '$o'")
  }

  def nextTime: Long
}

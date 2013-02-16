package com.sos.scheduler.engine.common.async

import com.google.inject.ImplementedBy
import java.util.concurrent.Callable
import scala.sys._

@ImplementedBy(classOf[PoppableCallQueue])
trait CallQueue {
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
    val removed = tryRemove(o)
    if (!removed) error(s"Unknown TimedCall '$o'")
  }

  def tryRemove(o: TimedCall[_]): Boolean
}
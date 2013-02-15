package com.sos.scheduler.engine.kernel.async

import com.sos.scheduler.engine.common.async.{TimedCall, CallQueue}
import java.util.concurrent.Callable

class SchedulerCallQueue(val delegate: CallQueue) {
  def add[A](f: => A) = delegate.add(f)

  def add(r: Runnable) = delegate.add(r)

  def add(r: Callable[_]) = delegate.add(r)

  def add(o: TimedCall[_]) =  delegate.add(o)

  def remove(o: TimedCall[_]) = delegate.remove(o)

  def tryRemove(o: TimedCall[_]) = delegate.tryRemove(o)
}

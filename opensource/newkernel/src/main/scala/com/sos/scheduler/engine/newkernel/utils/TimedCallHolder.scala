package com.sos.scheduler.engine.newkernel.utils

import com.sos.scheduler.engine.common.async.{TimedCall, CallQueue}
import org.joda.time.Instant
import scala.util.Try

final class TimedCallHolder(callQueue: CallQueue) {
  self =>
  private var call: TimedCall[Unit] = null

  def enqueue(at: Instant, caption: String = "")(f: => Unit): Unit = {
    if (call != null)
      callQueue tryCancel call

    call = new TimedCall[Unit] {
      def epochMillis = at.getMillis
      def call() = f
      override def onComplete(o: Try[Unit]): Unit = {
        self.call = null
        super.onComplete(o)
      }
      override def toStringPrefix = if (caption.nonEmpty) caption else super.toStringPrefix
    }
    callQueue add call
  }

  def close(): Unit = {
    cancel()
  }

  def cancel(): Unit = {
    if (call != null) {
      callQueue tryCancel call
      call = null
    }
  }
}

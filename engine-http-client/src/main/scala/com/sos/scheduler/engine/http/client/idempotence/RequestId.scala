package com.sos.scheduler.engine.http.client.idempotence

import java.util.concurrent.atomic.{AtomicReference, AtomicLong}
import java.lang.Math.addExact

/**
  * @author Joacim Zschimmer
  */
final case class RequestId(number: Long) {
  def succ = RequestId(addExact(number, 1))
  def pred = RequestId(addExact(number, -1))
}

object RequestId {
  def fromString(o: String) = RequestId(o.toLong)

  final class Generator extends (() ⇒ RequestId) {
    private val atomic = new AtomicLong

    def apply() = RequestId(atomic.incrementAndGet())
  }

  final class Eater extends (RequestId ⇒ Boolean) {
    private val ref = new AtomicReference[AtomicLong]

    def apply(id: RequestId): Boolean =
      ref.get == null && ref.compareAndSet(null, new AtomicLong(addExact(id.number, 1))) ||
        ref.get.compareAndSet(id.number, addExact(id.number, 1))

    def expectedId = RequestId(ref.get.get)
  }
}

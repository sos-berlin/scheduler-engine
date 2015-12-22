package com.sos.scheduler.engine.kernel.processclass.common.selection

import com.sos.scheduler.engine.kernel.processclass.common.EndlessRepeatedIterator._
import scala.collection.immutable

/**
  * @author Joacim Zschimmer
  */
final class RoundRobin[A](items: immutable.Seq[A]) extends Selector[A] {
  val iterator = endlessRepeatedIterator { items.iterator } .buffered
}

object RoundRobin extends SelectionMethod {
  def newSelector[A](items: immutable.Seq[A]) = new RoundRobin(items)
}

package com.sos.scheduler.engine.kernel.processclass.common.selection

import scala.collection.immutable

/**
  * Always the same selection order: the original order.
  *
  * @author Joacim Zschimmer
  */
final class FixedPriority[A](items: immutable.Seq[A]) extends Selector[A] {
  def iterator = items.iterator.buffered
}

object FixedPriority extends SelectionMethod {
  def newSelector[A](items: immutable.Seq[A]) = new FixedPriority(items)
}

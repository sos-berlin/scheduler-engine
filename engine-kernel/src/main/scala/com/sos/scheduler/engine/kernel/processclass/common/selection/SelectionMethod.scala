package com.sos.scheduler.engine.kernel.processclass.common.selection

import scala.collection.immutable

/**
  * @author Joacim Zschimmer
  */
trait SelectionMethod {
  def newSelector[A](items: immutable.Seq[A]): Selector[A]

  def name: String =
    getClass.getSimpleName stripSuffix "$"
}

package com.sos.scheduler.engine.kernel.processclass.common

/**
 * @author Joacim Zschimmer
 */
object EndlessRepeatedIterator {
  def endlessRepeatedIterator[A](newIterator: ⇒ Iterator[A]): Iterator[A] = {
    var i = newIterator
    Iterator.continually {
      if (!i.hasNext) {
        i = newIterator
      }
      i.next()
    }
  }
}

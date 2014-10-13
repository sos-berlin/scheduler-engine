package com.sos.scheduler.engine.kernel.processclass.common

import com.sos.scheduler.engine.kernel.processclass.common.EndlessRepeatedIterator._
import org.scalatest.FreeSpec

/**
 * @author Joacim Zschimmer
 */
final class EndlessRepeatedIteratorTest extends FreeSpec {

  "endlessRepeatedIterator fails for empty iterator" in {
    intercept[NoSuchElementException] {
      endlessRepeatedIterator(Iterator()).next()
    }
  }

  "endlessRepeatedIterator" in {
    assertResult(List(1, 2, 3, 1, 2, 3, 1, 2, 3)) {
      (endlessRepeatedIterator(Iterator(1, 2, 3)) take 9).toStream
    }
  }
}

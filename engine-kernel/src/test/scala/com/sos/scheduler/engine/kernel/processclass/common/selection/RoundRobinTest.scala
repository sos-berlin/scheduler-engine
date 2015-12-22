package com.sos.scheduler.engine.kernel.processclass.common.selection

import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class RoundRobinTest extends FreeSpec {

  "RoundRobin" in {
    val selector = RoundRobin.newSelector(List(1, 2, 3))
    assert((selector.iterator take 3).toList == List(1, 2, 3))
    assert((selector.iterator take 2).toList == List(1, 2))
    assert(selector.iterator.head == 3)
    assert((selector.iterator take 3).toList == List(3, 1, 2))
    assert((selector.iterator take 2).toList == List(3, 1))
    assert((selector.iterator take 2).toList == List(2, 3))
  }
}

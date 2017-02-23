package com.sos.scheduler.engine.kernel.order.jobchain

import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.common.time.Stopwatch
import com.sos.scheduler.engine.data.jobchain.NodeId
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain.allPredecessors
import com.sos.scheduler.engine.kernel.order.jobchain.JobChainTest._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Inspectors.forAll
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JobChainTest extends FreeSpec {

  "allPredecessors" in {
     forAll(TestData) { case T(n, from, expectedResult) ⇒
       forAll(n to Edges.size map Edges.take) { edges ⇒
         //info(s"$from $edges => $expectedResult")
         forAll(edges.permutations.toList) { p ⇒
           assertResult(expectedResult) { allPredecessors(p, from) }
         }
       }
     }
  }

  "allPredecessors performance (no test)" in {
    for (n ← List(1, 10, 100, 200, 300, 400, 500, 1000)) {
      val edges = for (i ← 1 until n) yield (i, i + 1)
      allPredecessors(edges, n)  // Warm-up
      val stopwatch = new Stopwatch
      val predecessors = allPredecessors(edges, n)
      logger.info(s"$n: $stopwatch")
      assert(predecessors.size == n - 1)
    }
  }

  "allPredecessors NodeId performance (no test)" in {
    val n = 1000
    val edges = 1 until n map { i ⇒ NodeId(s"$i") → NodeId(s"${i + 1}") }
    allPredecessors(edges, NodeId(s"$n"))  // Warm-up
    val stopwatch = new Stopwatch
    val predecessors = allPredecessors(edges, NodeId(s"$n"))
    logger.info(s"$n $stopwatch")
    assert(predecessors.size == n - 1)
  }
}

private object JobChainTest {
  private val logger = Logger(getClass)
  private val Edges = List(1 → 2, 2 → 3, 3 → 4,  7 → 8, 8 → 9)
  private val TestData = List(
    T(0, 1, Set()),
    T(1, 2, Set(1)),
    T(2, 3, Set(1, 2)),
    T(3, 4, Set(1, 2, 3)),
    T(5, 7, Set()),
    T(5, 8, Set(7)),
    T(5, 9, Set(7, 8)))

  private case class T(
    pairCount: Int,  // Präfix von Edges, 0..5, also Umfang des Graphs
    from: Int,
    expectedResult: Set[Int])
}

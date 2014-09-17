package com.sos.scheduler.engine.kernel.order.jobchain

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
}

private object JobChainTest {
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

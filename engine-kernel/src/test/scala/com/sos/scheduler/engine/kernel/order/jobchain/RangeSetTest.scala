package com.sos.scheduler.engine.kernel.order.jobchain

import com.sos.jobscheduler.common.scalautil.DuplicateKeyException
import com.sos.scheduler.engine.kernel.order.jobchain.RangeSet.InclusiveRange
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class RangeSetTest extends FreeSpec {

  "Parser" - {
    List[(String, List[InclusiveRange])](
      "1" → List(InclusiveRange(1)),
      "1..3" → List(InclusiveRange(1, 3)),
      "1..3 5" → List(InclusiveRange(1, 3), InclusiveRange(5)),
      "1..3 5 -3..0 -9..-7" → List(InclusiveRange(1, 3), InclusiveRange(5), InclusiveRange(-3, 0), InclusiveRange(-9, -7)),
      "1  2" → List(InclusiveRange(1), InclusiveRange(2)),
      "  1   2  " → List(InclusiveRange(1), InclusiveRange(2)))
    .foreach { case (string, ranges) ⇒
        string in {
          RangeSet(string).inclusiveRanges shouldEqual ranges
        }
     }
  }

  "Parser rejections" in {
    intercept[IllegalArgumentException] { RangeSet("2..1").inclusiveRanges }
    intercept[DuplicateKeyException] { RangeSet("1 1").inclusiveRanges }
    intercept[DuplicateKeyException] { RangeSet("1..2 3 2").inclusiveRanges }
    intercept[NumberFormatException] { RangeSet("").inclusiveRanges }
    intercept[NumberFormatException] { RangeSet("x").inclusiveRanges }
  }

  "contains" in {
    val r = RangeSet("3 9..10 -7..1 ")
    val set = Set(3, 9, 10) ++ (-7 to 1)
    for (i ← set) assert(r contains i)
    for (i ← -100 to +100 filterNot set) assert(!(r contains i))
  }

  "Int.MaxValue and Int.MinValue" in {
    for (i ← List(Int.MaxValue, Int.MinValue)) assert(!(RangeSet("1") contains i))
    RangeSet(Int.MaxValue.toString) contains Int.MaxValue
    RangeSet(Int.MinValue.toString) contains Int.MinValue
  }
}

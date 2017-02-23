package com.sos.scheduler.engine.kernel.order.jobchain

import com.sos.jobscheduler.common.scalautil.DuplicateKeyException
import com.sos.scheduler.engine.kernel.order.jobchain.RangeSet._

/**
 * @author Joacim Zschimmer
 */
private[jobchain] final case class RangeSet private(inclusiveRanges: Seq[InclusiveRange]) {
  for (Seq(a, b) ← inclusiveRanges sortBy { _.from } sliding 2) if (a.to >= b.from) throw new DuplicateKeyException(s"Overlapping ranges: $a $b")

  def contains(i: Int) = inclusiveRanges exists { _ contains i }

  override def toString = inclusiveRanges mkString " "
}

private[jobchain] object RangeSet {
  def apply(string: String): RangeSet = new RangeSet(string.trim split " +" map InclusiveRange.apply)

  private[jobchain] case class InclusiveRange(from: Int, to: Int) {
    require(from <= to, s"Reverse range is invalid: $this")
    def contains(i: Int) = from <= i && i <= to
    override def toString = if (from == to) s"$from" else s"$from..$to"
  }

  private[jobchain] object InclusiveRange {
    private val Separator = ".."
    def apply(o: String): InclusiveRange = {
      o indexOf Separator match {
        case -1 ⇒ apply(o.toInt)
        case i ⇒ apply(o.substring(0, i).toInt, o.substring(i + Separator.size).toInt)
      }
    }

    def apply(i: Int): InclusiveRange = new InclusiveRange(i, i)
  }
}

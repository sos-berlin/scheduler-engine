package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.common.time.Stopwatch
import com.sos.scheduler.engine.data.order.OrderSourceType
import com.sos.scheduler.engine.kernel.order.DatabaseOrders.OrderXmlResolved
import java.io.StringReader
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class DatabaseOrdersTest extends FreeSpec {

  "OrderXmlResolved" - {
    "Empty XML" in {
      val orderXml = <order/>
      assert(OrderXmlResolved(new StringReader(orderXml.toString)) == OrderXmlResolved(
        isSuspended = false,
        isBlacklisted = false,
        isSetback = false,
        isTouched = false,
        sourceType = OrderSourceType.AdHoc))
    }

    "Full file order XML" in {
      val orderXml =
        <order suspended="true" setback="2016-10-04T11:22:33Z" touched="true" on_blacklist="true">
          <params>
            <param name="scheduler_file_path" value="PATH"/>
          </params>
        </order>
        assert(OrderXmlResolved(new StringReader(orderXml.toString)) == OrderXmlResolved(
          isSuspended = true,
          isBlacklisted = true,
          isSetback = true,
          isTouched = true,
          sourceType = OrderSourceType.FileOrder))
    }

    "Permament order" in {
      val orderXml =
        <order>
          <file_based last_write_time="2016-10-04T11:22:33Z"/>
        </order>
      assert(OrderXmlResolved(new StringReader(orderXml.toString)) == OrderXmlResolved(
        isSuspended = false,
        isBlacklisted = false,
        isSetback = false,
        isTouched = false,
        sourceType = OrderSourceType.Permanent))
    }

    if (false)
    "Speed test" - {
      val textSize = 4*1000*1000
      for ((lineCount, lineLength, iterations) ← Array(
        (1, textSize, 100),
        (textSize, 1, 100),
        (1, 100, 10000)))
      {
        s"$lineCount lines with $lineLength characters each" in {
          val text = ("X" * (lineLength - 1) + "\n") * lineCount
          val orderXml =
            <order suspended="true" setback="2016-10-04T11:22:33Z" touched="true" on_blacklist="true">
              <file_based last_write_time="2016-10-04T11:22:33Z"/>
              <params>
                <param name="a" value="A"/>
                <param name="b" value="B"/>
                <param name="c" value="C"/>
                <param name="d" value="D"/>
                <param name="e" value="E"/>
                <param name="f" value="F"/>
                <param name="g" value="G"/>
                <param name="h" value="H"/>
                <param name="i" value="I"/>
                <param name="j" value="J"/>
              </params>
              <log>{text}</log>
            </order>.toString
          for (_ ← 1 to 5) Stopwatch.measureTime(iterations, s"XML(${lineCount}x$lineLength)") {
            OrderXmlResolved(new StringReader(orderXml))
          }
        }
      }
    }
  }
}

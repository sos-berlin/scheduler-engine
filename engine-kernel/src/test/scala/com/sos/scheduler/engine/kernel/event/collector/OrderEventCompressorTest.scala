package com.sos.scheduler.engine.kernel.event.collector

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.data.event.Event
import com.sos.scheduler.engine.data.filebased.{FileBasedActivated, FileBasedAdded, FileBasedRemoved}
import com.sos.scheduler.engine.data.jobchain.NodeId
import com.sos.scheduler.engine.data.order.{OrderEvent, OrderFinished, OrderNodeChanged, OrderResumed, OrderStarted, OrderStepEnded, OrderStepStarted}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.collection.{immutable, mutable}

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class OrderEventCompressorTest extends FreeSpec {

  "test" in {

  }
}

object OrderEventCompressorTest {
  private val logger = Logger(getClass)

  private def compress(events: immutable.Seq[Event]): immutable.Seq[Event] = {
    val result = mutable.Buffer[Event]()
    for (e ← events.reverseIterator) {
      e match {
        case FileBasedRemoved ⇒ // Alle vorherigen verwerfen
        case _: OrderFinished ⇒ Set()
      }
    }
    result.toVector
  }

  private val EventOverridesMap = Map[Class[_ <: OrderEvent], Set[Class[_ <: OrderEvent]]](
    OrderStarted.getClass → Set(
      classOf[OrderNodeChanged],
      OrderResumed.getClass),
    classOf[OrderFinished] → Set(),
    classOf[OrderStepStarted] → Set(
      OrderStarted.getClass,
      classOf[OrderNodeChanged]),
    classOf[OrderStepEnded] → Set(
      OrderStarted.getClass,
      classOf[OrderNodeChanged]),
    classOf[OrderNodeChanged] → Set(
      OrderStarted.getClass))

  private val TestEvents = List(
    FileBasedAdded,
    FileBasedActivated,
    OrderStarted,
    OrderStepStarted,
    OrderStepEnded,
    OrderNodeChanged,
    OrderStepStarted,
    OrderStepEnded,
    OrderFinished(NodeId("END")),
    OrderNodeChanged)
}

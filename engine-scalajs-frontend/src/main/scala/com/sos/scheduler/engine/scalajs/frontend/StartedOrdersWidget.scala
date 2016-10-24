package com.sos.scheduler.engine.scalajs.frontend

import com.sos.scheduler.engine.scalajs.frontend.StartedOrdersWidget._
import com.sos.scheduler.engine.scalajs.frontend.Utils._
import org.scalajs.dom
import org.scalajs.dom.window
import org.scalajs.dom.html
import scala.collection.mutable
import scala.scalajs.concurrent.JSExecutionContext.Implicits.queue
import scala.scalajs.js
import scala.scalajs.js.annotation.JSExport
import scala.util.{Failure, Success}
import scalatags.JsDom.StringFrag
import scalatags.JsDom.all._

/**
  * @author Joacim Zschimmer
  */
@JSExport
final class StartedOrdersWidget {

  private val orderMap = mutable.Map[OrderKey, OrderEntry]()
  private val orderTbody = tbody.render
  private val orderTable =
    table(cls := "table table-condensed table-hover")(
      thead(
        th("OrderKey"),
        th("NodeId"),
        th("TaskId")),
      orderTbody)
    .render

  def start(): html.Element = {
    refreshStartedOrders()
    orderTable
  }

  private def refreshStartedOrders(): Unit = {
    val responded = getDynamic("api/order/?return=OrderOverview&isOrderProcessingState=WaitingInTask,InTaskProcess&isDistributed=false")
    while (orderTbody.hasChildNodes) {
      orderTbody.removeChild(orderTbody.firstChild)
    }
    orderMap.clear()
    (for (responseSnapshot ← responded) yield {
      for (orderView ← responseSnapshot.orders.asInstanceOf[js.Array[js.Dynamic]]) {
        val orderKey = orderView.path.asInstanceOf[String]
        val nodeIdElem = StringFrag(orderView.nodeId.asInstanceOf[String]).render
        val taskIdElem = StringFrag(orderView.orderProcessingState.taskId.toString).render
        val rowElem = tr(td(orderKey), td(nodeIdElem), td(taskIdElem)).render
        orderTbody.insertBefore(rowElem, orderTbody.firstElementChild)
        orderMap += orderKey → OrderEntry(rowElem, nodeIdElem, taskIdElem)
      }
      getEvents(responseSnapshot.eventId.asInstanceOf[Double].toLong)
    }) onFailure {
      case t ⇒
        window.console.error(t.toString)
        window.setTimeout(refreshStartedOrders _, 5000)
    }
  }

  private def getEvents(after: EventId): Unit = {
    getDynamic(s"api/order/?return=Event&timeout=60s&after=$after") onComplete {
      case Success(responseSnapshot) ⇒
        responseSnapshot.TYPE.asInstanceOf[String] match {
          case "NonEmpty" ⇒
            val events = responseSnapshot.eventSnapshots.asInstanceOf[js.Array[js.Dynamic]]
            events foreach handleOrderEvent
            val lastEventId = events.last.eventId.asInstanceOf[Double].toLong
            getEvents(lastEventId)
          case "Empty" ⇒
            getEvents(responseSnapshot.lastEventId.asInstanceOf[Double].toLong)
          case "Torn" ⇒
            refreshStartedOrders()
        }
      case Failure(t) ⇒
        window.console.error(t.toString)
        window.setTimeout(refreshStartedOrders _, 5000)
    }
  }

  private def handleOrderEvent(event: js.Dynamic): Unit = {
    val orderKey = event.key.asInstanceOf[String]
    event.TYPE.asInstanceOf[String] match {
      case "FileBasedRemoved" ⇒
      case "OrderStarted" ⇒
        for (orderEntry ← orderMap.remove(orderKey)) {
          orderTbody.removeChild(orderEntry.rowElem)
        }
        val nodeIdElem = StringFrag("").render
        val taskIdElem = StringFrag("").render
        val rowElem = tr(td(orderKey.toString), td(nodeIdElem), td(taskIdElem)).render
        orderTbody.insertBefore(rowElem, orderTbody.firstElementChild)
        orderMap += orderKey → OrderEntry(rowElem, nodeIdElem, taskIdElem)
      case "OrderFinished" ⇒
        for (orderEntry ← orderMap.remove(orderKey)) {
          orderTbody.removeChild(orderEntry.rowElem)
        }
      case "OrderStepStarted" ⇒
        for (orderEntry ← orderMap.get(orderKey)) {
          orderEntry.nodeIdElem.textContent = event.nodeId.toString
          orderEntry.taskIdElem.textContent = event.taskId.toString
        }
      case "OrderStepEnded" ⇒
        for (orderEntry ← orderMap.get(orderKey)) {
          orderEntry.taskIdElem.textContent = ""
        }
      case "OrderNodeChanged" ⇒
        for (orderEntry ← orderMap.get(orderKey)) {
          orderEntry.nodeIdElem.textContent = event.nodeId.toString
        }
      case _ ⇒
    }
  }
}

object StartedOrdersWidget {
  private type OrderKey = String

  private case class OrderEntry(rowElem: html.TableRow, nodeIdElem: dom.Text, taskIdElem: dom.Text)
}

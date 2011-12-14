package com.sos.scheduler.engine.tests.order.events

import org.junit.Test
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.kernel.folder.AbsolutePath
import com.sos.scheduler.engine.kernel.order._

//TODO Testet nicht, ob zu viele Events eintreffen
/** Testet, ob die erwarteten Events eintreffen. */
class OrderEventsTest extends ScalaSchedulerTest {
    import OrderEventsTest._

    private val eventPipe = controller.newEventPipe()

    @Test def test1() {
        controller.activateScheduler()
        testPersistentOrder()
        testTemporaryOrder()
        testRemovedOrder(temporaryOrderKey)
    }

    def testPersistentOrder() {
        checkOrderStates(persistentOrderKey)
    }

    def testPersistentOrder2() {
        checkOrderStates(persistentOrderKey)
    }

    def testTemporaryOrder() {
        scheduler.executeXml(<add_order job_chain={jobChainPath.toString} id={temporaryOrderKey.getId.toString}/>)
        checkOrderStates(temporaryOrderKey)
    }

    def testRemovedOrder(orderKey: OrderKey) {
        scheduler.executeXml(<add_order job_chain={jobChainPath.toString} id={orderKey.getId.toString}/>)
        eventPipe.expectEvent(shortTimeout) { e: OrderTouchedEvent => e.getKey == orderKey }
        eventPipe.expectEvent(shortTimeout) { e: OrderSuspendedEvent => e.getKey == orderKey }
        scheduler.executeXml(<remove_order job_chain={jobChainPath.toString} order={orderKey.getId.toString}/>)
        //eventPipe.expectEvent(shortTimeout) { e: OrderFinishedEvent => e.getKey == orderKey }
    }

    private def checkOrderStates(orderKey: OrderKey) {
        eventPipe.expectEvent(shortTimeout) { e: OrderTouchedEvent => e.getKey == orderKey }
        eventPipe.expectEvent(shortTimeout) { e: OrderSuspendedEvent => e.getKey == orderKey }
        scheduler.executeXml(<modify_order job_chain={jobChainPath.toString} order={orderKey.getId.toString} suspended="no"/>)
        eventPipe.expectEvent(shortTimeout) { e: OrderResumedEvent => e.getKey == orderKey }
        eventPipe.expectEvent(shortTimeout) { e: OrderFinishedEvent => e.getKey == orderKey }
    }
}

object OrderEventsTest {
    private val jobChainPath = new AbsolutePath("/a")
    private val persistentOrderKey = new OrderKey(jobChainPath, new OrderId("persistentOrder"))
    private val temporaryOrderKey = new OrderKey(jobChainPath, new OrderId("1"))
}

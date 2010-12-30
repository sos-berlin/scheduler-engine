package com.sos.scheduler.kernel.core_cpp_test.order.endstate

import com.sos.scheduler.kernel.core.folder.AbsolutePath
import com.sos.scheduler.kernel.core.order.OrderId
import com.sos.scheduler.kernel.core.order.OrderState
import com.sos.scheduler.kernel.core.util.Time
import com.sos.scheduler.kernel.core_cpp_test._
import org.junit._


class OrderEndStateTest extends ScalaSchedulerTest {
    private val eventTimeout = Time.of(3)

    @Test def test1() {
//        startScheduler()
//        val scheduler = waitUntilSchedulerIsRunning()
//        val jobChain = scheduler.getOrderSubsystem.jobChain(new AbsolutePath("/myJobChain"))
//        val order = jobChain.order(new OrderId("id.1"))
//        order.setEndState(new OrderState(""))
//        order.setEndState(new OrderState("state.end"))
//        //...
//        scheduler.terminate()
    }
}

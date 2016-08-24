package com.sos.scheduler.engine.kernel.event

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.event.AnyKeyedEvent
import com.sos.scheduler.engine.eventbus.{EventSource, SchedulerEventBus}
import com.sos.scheduler.engine.kernel.event.EventSubsystem._
import com.sos.scheduler.engine.kernel.scheduler.Subsystem
import javax.inject.{Inject, Singleton}

@ForCpp @Singleton
private[kernel] final class EventSubsystem @Inject private(eventBus: SchedulerEventBus) extends Subsystem {

  /** @param e [[com.sos.scheduler.engine.data.event.AnyKeyedEvent]] statt [[com.sos.scheduler.engine.data.event.AnyKeyedEvent]],
    *          weil C++/Java-Generator die Interface-Hierarchie nicht berücksichtig. */
  @ForCpp private def report(e: AnyKeyedEvent): Unit = {
    eventBus.publish(e)
  }

  @ForCpp private def checkNumberOfEventCodes(count: Int): Unit = {
    require(count == CppEventCode.values.length, "C++-Event_code does not match CppEventCode")
  }

  @ForCpp private def reportEventClass(cppEventCode: Int, eventSource: AnyRef): Unit = {
    try {
      val o = eventSource.asInstanceOf[EventSource]
      val e = CppEventFactory.newInstance(CppEventCode.values()(cppEventCode), o)
      eventBus.publish(e)
    }
    catch {
      case x: Exception => logger.error(s"EventSubsystem.reportEventClass($cppEventCode):", x)
    }
  }

  override def toString =
    getClass.getSimpleName
}


object EventSubsystem {
  private val logger = Logger(getClass)
}

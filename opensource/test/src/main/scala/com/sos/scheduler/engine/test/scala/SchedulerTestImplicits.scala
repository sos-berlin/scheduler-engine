package com.sos.scheduler.engine.test.scala

import scala.xml.Elem
import com.sos.scheduler.engine.kernel.Scheduler

object SchedulerTestImplicits {
    implicit def toRichScheduler(scheduler: Scheduler) = new {
        def executeXml(e: Elem): Elem = xml.XML.loadString(scheduler.executeXml(e.toString()))
    }
}

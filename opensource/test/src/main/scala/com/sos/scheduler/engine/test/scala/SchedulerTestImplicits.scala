package com.sos.scheduler.engine.test.scala

import scala.xml.Elem
import com.sos.scheduler.engine.kernel.Scheduler

object SchedulerTestImplicits {
  implicit def toRichScheduler(scheduler: Scheduler) = new ScaledScheduler(scheduler)

  class ScaledScheduler(scheduler: Scheduler) {
    def executeXml(e: Elem): Elem = xml.XML.loadString(scheduler.executeXml(e.toString()))
    def instance[A](implicit m: Manifest[A]): A = scheduler.getInjector.getInstance(m.erasure).asInstanceOf[A]
  }
}

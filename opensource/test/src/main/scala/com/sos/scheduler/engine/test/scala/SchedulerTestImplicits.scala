package com.sos.scheduler.engine.test.scala

import scala.xml.Elem
import com.sos.scheduler.engine.kernel.Scheduler
import scala.reflect.ClassTag

object SchedulerTestImplicits {
  implicit def toRichScheduler(scheduler: Scheduler) = new ScaledScheduler(scheduler)

  class ScaledScheduler(scheduler: Scheduler) {
    def executeXml(e: Elem): Elem = xml.XML.loadString(scheduler.executeXml(e.toString()))
    def instance[A](implicit c: ClassTag[A]): A = scheduler.getInjector.getInstance(c.runtimeClass).asInstanceOf[A]
  }
}

package com.sos.scheduler.engine.test.scala

import scala.xml.{NodeBuffer, Node, NodeSeq, Elem}
import com.sos.scheduler.engine.kernel.Scheduler
import scala.reflect.ClassTag
import scala.xml
import scala.xml
import com.sos.scheduler.engine.common.Lazy

object SchedulerTestImplicits {
  implicit class ScaledScheduler(val scheduler: Scheduler) extends AnyVal {
    def executeXmls(e: Iterable[NodeBuffer]): Result =
      executeXmlString(<commands>{e}</commands>.toString())

    def executeXmls(e: NodeSeq): Result =
      executeXmlString(<commands>{e}</commands>.toString())

    def executeXml(e: Elem): Result =
      executeXmlString(e.toString())

    private def executeXmlString(o: String): Result = {
      val result = scheduler.executeXml(o)
      Result(result)
    }

    def instance[A](implicit c: ClassTag[A]): A = scheduler.injector.getInstance(c.runtimeClass).asInstanceOf[A]
  }

  case class Result(string: String) {
    lazy val elem = xml.XML.loadString(string)
  }
}
package com.sos.scheduler.engine.test.scala

import com.sos.scheduler.engine.common.scalautil.xmls.SafeXML
import com.sos.scheduler.engine.data.xmlcommands.XmlCommand
import com.sos.scheduler.engine.kernel.Scheduler
import scala.reflect.ClassTag

object SchedulerTestImplicits {
  implicit class ScaledScheduler(val scheduler: Scheduler) extends AnyVal {
    def executeXmls(e: Iterable[xml.NodeBuffer]): Result =
      executeXmlString(<commands>{e}</commands>.toString())

    def executeXml(o: XmlCommand): Result =
      executeXmlString(o.xmlString)

    def executeXmls(e: xml.NodeSeq): Result =
      executeXmlString(<commands>{e}</commands>.toString())

    def executeXml(e: xml.Elem): Result =
      executeXmlString(e.toString())

    private def executeXmlString(o: String) =
      Result(scheduler.executeXml(o))

    def instance[A](implicit c: ClassTag[A]): A =
      scheduler.injector.getInstance(c.runtimeClass).asInstanceOf[A]
  }

  final case class Result(string: String) {
    lazy val elem: xml.Elem =
      SafeXML.loadString(string)

    lazy val answer =
      elem \ "answer"
  }
}

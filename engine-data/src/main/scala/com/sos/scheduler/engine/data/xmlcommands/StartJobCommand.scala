package com.sos.scheduler.engine.data.xmlcommands

import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.time.SchedulerDateTime
import com.sos.scheduler.engine.data.xmlcommands.StartJobCommand._
import java.time.Instant

final case class StartJobCommand(jobPath: JobPath, variables: Iterable[(String, String)] = Nil, at: Option[At] = None) extends XmlCommand {
  def xmlElem =
    <start_job job={jobPath.string} at={(at map { _.toXmlValue }).orNull}><params>{
      variables map { case (k, v) ⇒ <param name={k} value={v}/> }
    }</params></start_job>
}

object StartJobCommand {
  sealed trait At {
    def toXmlValue: String
  }
  object At {
    def apply(instant: Instant): At = new At {
      def toXmlValue = SchedulerDateTime.formatUtc(instant)
    }

    object Period extends At {
      def toXmlValue = "period"
    }
  }
}

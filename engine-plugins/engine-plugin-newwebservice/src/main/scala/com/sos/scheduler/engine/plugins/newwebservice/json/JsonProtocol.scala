package com.sos.scheduler.engine.plugins.newwebservice.json

import com.sos.scheduler.engine.data.filebased.FileBasedDetailed
import com.sos.scheduler.engine.data.jobchain.JobChainDetailed
import com.sos.scheduler.engine.kernel.filebased.{FileBasedSubsystemOverview, SimpleFileBasedDetailed, SimpleFileBasedSubsystemOverview}
import com.sos.scheduler.engine.kernel.job.JobSubsystemOverview
import spray.json._

object JsonProtocol extends DefaultJsonProtocol {

  implicit object FileBasedSubsystemOverviewJsonWriter extends RootJsonWriter[FileBasedSubsystemOverview] {
    def write(o: FileBasedSubsystemOverview) = o match {
      case o: JobSubsystemOverview ⇒ o.toJson
      case o: SimpleFileBasedSubsystemOverview ⇒ o.toJson
    }
  }

  implicit object FileBasedDetailedJsonWriter extends RootJsonWriter[FileBasedDetailed] {
    def write(o: FileBasedDetailed) = o match {
      case o: SimpleFileBasedDetailed ⇒ o.toJson
      case o: JobChainDetailed ⇒ o.toJson
    }
  }
}

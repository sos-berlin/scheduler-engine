package com.sos.scheduler.engine.plugins.newwebservice.json

import com.sos.scheduler.engine.data.filebased.FileBasedDetails
import com.sos.scheduler.engine.data.jobchain.JobChainDetails
import com.sos.scheduler.engine.kernel.filebased.{FileBasedSubsystemOverview, SimpleFileBasedDetails, SimpleFileBasedSubsystemOverview}
import com.sos.scheduler.engine.kernel.job.JobSubsystemOverview
import spray.json._

object JsonProtocol extends DefaultJsonProtocol {

  implicit object FileBasedSubsystemOverviewJsonWriter extends RootJsonWriter[FileBasedSubsystemOverview] {
    def write(o: FileBasedSubsystemOverview) = o match {
      case o: JobSubsystemOverview ⇒ o.toJson
      case o: SimpleFileBasedSubsystemOverview ⇒ o.toJson
    }
  }

  implicit object FileBasedDetailsJsonWriter extends RootJsonWriter[FileBasedDetails] {
    def write(o: FileBasedDetails) = o match {
      case o: SimpleFileBasedDetails ⇒ o.toJson
      case o: JobChainDetails ⇒ o.toJson
    }
  }
}

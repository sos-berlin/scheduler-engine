package com.sos.scheduler.engine.newkernel.job

import com.sos.scheduler.engine.newkernel.schedule.Schedule
import com.sos.scheduler.engine.data.folder.AbsolutePath
import scala.sys.error

case class JobConfiguration(
    script: JobScript,
    schedule: Schedule,
    //idleTimeout: Option[Duration],
    //minimumTasks: Option[Int],
    //maximumTasks: Option[Int],
    //javaOptions: Option[String],
    title: String) {

  def processClassPath: AbsolutePath = ???
}

object JobConfiguration {
  class Builder {
    var title = ""
    var description = ""
    var script: Option[JobScript] = None
    var schedule: Option[Schedule] = None

    def build() = new JobConfiguration(
      script getOrElse error("Missing 'script'"),
      schedule getOrElse Schedule.Default,
      title)
  }
}
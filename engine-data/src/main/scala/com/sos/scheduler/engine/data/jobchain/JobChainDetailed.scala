package com.sos.scheduler.engine.data.jobchain

import com.sos.scheduler.engine.base.sprayjson.JavaTimeJsonFormats.implicits._
import com.sos.scheduler.engine.base.sprayjson.SprayJson.JsonFormats._
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.order.OrderOverview
import java.nio.file.Path
import java.time.{Duration, Instant}
import scala.collection.immutable.Seq
import spray.json.DefaultJsonProtocol._

final case class JobChainDetailed(
  overview: JobChainOverview,
  nodes: Seq[NodeOverview],
  fileOrderSources: Seq[JobChainDetailed.FileOrderSource] = Nil,
  blacklistedOrders: Seq[OrderOverview] = Nil)

object JobChainDetailed
{
  final case class FileOrderSourceFile(
    file: Path,
    lastModified: Instant)
  object FileOrderSourceFile {
    implicit val JsonFormat = jsonFormat2(apply)
  }

  final case class FileOrderSource(
    directory: Path,
    regex: String,
    repeat: Duration,
    delayAfterError: Duration,
    alertWhenDirectoryMissing: Boolean,
    files: Seq[FileOrderSourceFile])
  object FileOrderSource {
    implicit val JsonFormat = jsonFormat6(apply)
  }

  private implicit val FileBasedStateJsonFormat = FileBasedState.MyJsonFormat
  implicit val MyJsonFormat = jsonFormat4(apply)
}

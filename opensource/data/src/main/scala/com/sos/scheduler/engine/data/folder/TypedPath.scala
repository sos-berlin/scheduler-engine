package com.sos.scheduler.engine.data.folder

import java.io.File
import scala.Predef.String

trait TypedPath
extends AbsolutePath {

  protected def typ: FileBasedType

  def file(baseDirectory: File): File =
    new File(baseDirectory, filename(string))

  def filename(name: String): String =
    if (typ eq FileBasedType.folder) name + "/"
    else name + "." + typ.cppName + ".xml"

  override def toString =
    s"$typ ${super.toString}"
}

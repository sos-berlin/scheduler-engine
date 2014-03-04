package com.sos.scheduler.engine.data.folder

import java.io.File
import scala.Predef.String

trait TypedPath
extends AbsolutePath {

  protected def typ: FileBasedType

  def file(baseDirectory: File): File =
    new File(baseDirectory, relativeFilePath)

  /** @return Relativer Pfad mit Schrägstrich beginnend. Großschreibung kann bei manchen Typen abweichen. */
  def relativeFilePath: String =
    if (typ eq FileBasedType.folder) string + "/"
    else string + "." + typ.cppName + ".xml"

  override def toString =
    s"$typ ${super.toString}"
}

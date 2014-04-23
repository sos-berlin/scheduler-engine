package com.sos.scheduler.engine.kernel.cppproxy

import com.sos.scheduler.engine.kernel.filebased.FileBased

trait SubsystemC[F <: FileBased, C <: File_basedC[F]]  {
  def java_file_based(path: String): C
  def java_file_based_or_null(path: String): C
  def java_active_file_based(path: String): C
  def java_file_baseds: java.util.ArrayList[F]
  def file_based_paths(visibleOnly: Boolean): Array[String]
  def is_empty: Boolean
}

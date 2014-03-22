package com.sos.scheduler.engine.kernel.cppproxy

import com.sos.scheduler.engine.cplusplus.runtime.Sister

trait File_basedC[S <: Sister] {
  def path: String
  def name: String
  def file_based_state_name: String
  def file: String
  def source_xml_bytes: Array[Byte]
  def log: Prefix_logC
  def set_force_file_reread()
  def is_file_based_reread: Boolean
  def is_visible: Boolean
  def has_base_file: Boolean
  def is_to_be_removed: Boolean
}

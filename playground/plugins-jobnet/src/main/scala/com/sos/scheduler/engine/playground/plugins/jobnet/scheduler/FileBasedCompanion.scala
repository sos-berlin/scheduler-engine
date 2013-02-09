package com.sos.scheduler.engine.playground.plugins.jobnet.scheduler

import com.sos.scheduler.engine.kernel.folder.FileBased
import com.sos.scheduler.engine.kernel.plugin.Plugin

trait FileBasedCompanion extends AutoCloseable {

  def close()

//  def fileBased: A
//
//  def plugin: Plugin
}

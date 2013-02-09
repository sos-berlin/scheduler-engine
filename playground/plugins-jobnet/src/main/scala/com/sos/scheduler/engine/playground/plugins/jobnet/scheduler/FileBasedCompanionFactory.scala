package com.sos.scheduler.engine.playground.plugins.jobnet.scheduler

import com.sos.scheduler.engine.kernel.folder.FileBased

trait FileBasedCompanionFactory {
  def newCompanion(fileBased: FileBased, configurationString: String): (FileBasedCompanion, String)
}

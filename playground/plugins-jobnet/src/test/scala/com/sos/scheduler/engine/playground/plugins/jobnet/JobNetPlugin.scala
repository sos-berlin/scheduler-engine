package com.sos.scheduler.engine.playground.plugins.jobnet

import com.sos.scheduler.engine.eventbus.EventBus
import com.sos.scheduler.engine.kernel.folder.FileBased
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin
import com.sos.scheduler.engine.playground.plugins.jobnet.scheduler.{FileBasedCompanionFactory, JobChain}
import javax.inject.Inject

final class JobNetPlugin @Inject private(eventBus: EventBus) extends AbstractPlugin with FileBasedCompanionFactory {

  override def activate() {}

  override def close() {}

  def newCompanion(fileBased: FileBased, configurationString: String) = {
    val jobChain = fileBased.asInstanceOf[JobChain]
    val (companion, jobChainXml) = JobChainCompanion.newCompanion(jobChain, configurationString, eventBus)
    (companion, jobChainXml)
  }
}
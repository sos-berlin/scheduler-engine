package com.sos.scheduler.engine.scalajs.plugin

import com.google.inject.AbstractModule
import com.google.inject.multibindings.Multibinder
import com.sos.scheduler.engine.plugins.newwebservice.ExtraRoute

/**
  * @author Joacim Zschimmer
  */
final class ScalajsModule extends AbstractModule {

  def configure() = Multibinder.newSetBinder(binder, classOf[ExtraRoute]).addBinding to classOf[ScalajsRoute]
}

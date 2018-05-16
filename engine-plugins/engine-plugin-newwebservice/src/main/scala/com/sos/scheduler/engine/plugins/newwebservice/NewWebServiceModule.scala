package com.sos.scheduler.engine.plugins.newwebservice

import com.google.inject.AbstractModule
import com.google.inject.multibindings.Multibinder

/**
  * @author Joacim Zschimmer
  */
final class NewWebServiceModule extends AbstractModule {
  override def configure() = {
    Multibinder.newSetBinder(binder, classOf[ExtraRoute])
  }
}

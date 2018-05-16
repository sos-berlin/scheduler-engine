package com.sos.scheduler.engine.kernel.plugin

import com.google.inject.TypeLiteral
import com.sos.scheduler.engine.common.guice.ScalaAbstractModule
import scala.collection.immutable

final class PluginModule(pluginConfigurations: immutable.Seq[PluginConfiguration])
extends ScalaAbstractModule  {

  override def configure(): Unit = {
    bind(new TypeLiteral[immutable.Seq[PluginConfiguration]] {}) toInstance pluginConfigurations
    pluginConfigurations flatMap { _.guiceModuleOption } foreach install
    bindInstance(pluginConfigurations)
  }
}

object PluginModule {
  def apply(xml: String) = new PluginModule(PluginConfiguration.readXml(xml))
}

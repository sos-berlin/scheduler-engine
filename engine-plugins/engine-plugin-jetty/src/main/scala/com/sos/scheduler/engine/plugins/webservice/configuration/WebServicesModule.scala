package com.sos.scheduler.engine.plugins.webservice.configuration

import WebServicesModule._
import com.google.inject.AbstractModule
import com.sos.scheduler.engine.plugins.webservice.services._
import scala.collection.immutable

class WebServicesModule extends AbstractModule {
  def configure(): Unit = {
    for (o <- services) bind(o)
  }
}

object WebServicesModule {
  private val services = immutable.Seq(
    classOf[CommandService])
}

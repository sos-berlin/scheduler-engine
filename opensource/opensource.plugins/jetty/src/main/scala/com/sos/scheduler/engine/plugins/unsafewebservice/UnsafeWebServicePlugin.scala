package com.sos.scheduler.engine.plugins.unsafewebservice

import UnsafeHttpGetCommandPlugin._
import com.google.inject.AbstractModule
import com.sos.scheduler.engine.kernel.plugin.{Plugin, UseGuiceModule}
import com.sos.scheduler.engine.common.scalautil.Logger

/**
 * @author Joacim Zschimmer
 */
@UseGuiceModule(classOf[WebServicesModule])
final class UnsafeHttpGetCommandPlugin extends Plugin

object UnsafeHttpGetCommandPlugin {
  private val logger = Logger(getClass)

  final class WebServicesModule extends AbstractModule {
    def configure(): Unit = {
      logger.warn("Use of UnsafeHttpGetCommandPlugin IS UNSAFE")
      bind(classOf[UnsafeCommandService])
    }
  }
}

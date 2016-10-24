package com.sos.scheduler.engine.scalajs.plugin

import com.sos.scheduler.engine.kernel.plugin.{Plugin, UseGuiceModule}

/**
  * @author Joacim Zschimmer
  */
@UseGuiceModule(classOf[ScalajsModule])
final class ScalajsPlugin extends Plugin

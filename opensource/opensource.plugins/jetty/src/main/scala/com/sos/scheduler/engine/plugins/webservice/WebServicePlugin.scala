package com.sos.scheduler.engine.plugins.webservice

import com.sos.scheduler.engine.kernel.plugin.{AbstractPlugin, UseGuiceModule}
import com.sos.scheduler.engine.plugins.webservice.configuration.WebServicesModule

@UseGuiceModule(classOf[WebServicesModule])
class WebServicePlugin extends AbstractPlugin

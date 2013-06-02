package com.sos.scheduler.engine.plugins.jetty.configuration.injection

import com.sos.scheduler.engine.plugins.jetty.configuration.Config._
import com.sos.scheduler.engine.plugins.jetty.cpp.CppServlet
import com.sos.scheduler.engine.plugins.jetty.log.{MainLogServlet, OrderLogServlet, JobLogServlet}
import com.sos.scheduler.engine.plugins.jetty.services.RootService

final class JettyModule extends JerseyModule {
  override def configureServlets() {
    super.configureServlets()
    configureEngineServlets()
    configureOldCppServlet()
    bind(classOf[RootService])
  }

  private def configureEngineServlets() {
    serve(s"$enginePrefixPath/log") `with` classOf[MainLogServlet]
    serveRegex(s"$enginePrefixPath/${JobLogServlet.PathInfoRegex}") `with` classOf[JobLogServlet]
    serveRegex(s"$enginePrefixPath/${OrderLogServlet.PathInfoRegex}") `with` classOf[OrderLogServlet]
  }

  private def configureOldCppServlet() {
    serve(cppPrefixPath) `with` classOf[CppServlet]
    serve(s"$cppPrefixPath/*") `with` classOf[CppServlet]
  }
}


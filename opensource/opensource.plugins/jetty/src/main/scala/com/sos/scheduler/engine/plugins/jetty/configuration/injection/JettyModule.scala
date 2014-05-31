package com.sos.scheduler.engine.plugins.jetty.configuration.injection

import JettyModule._
import com.sos.scheduler.engine.plugins.jetty.configuration.Config._
import com.sos.scheduler.engine.plugins.jetty.cpp.CppServlet
import com.sos.scheduler.engine.plugins.jetty.log.{MainLogServlet, OrderLogServlet, JobLogServlet}
import javax.inject.Singleton
import javax.ws.rs.core._
import javax.ws.rs.{GET, Path}

final class JettyModule extends JerseyModule {
  override def configureServlets() {
    configureEngineServlets()
    configureOldCppServlet()
    super.configureServlets()
    bind(classOf[DummyService])
  }

  private def configureEngineServlets() {
    serve(s"$enginePrefixPath/log") `with` classOf[MainLogServlet]
    serve(s"$enginePrefixPath/job.log") `with` classOf[JobLogServlet]
    serve(s"$enginePrefixPath/order.log") `with` classOf[OrderLogServlet]
  }

  private def configureOldCppServlet() {
    serve(s"$cppPrefixPath") `with` classOf[CppServlet]
    serve(s"$cppPrefixPath/*") `with` classOf[CppServlet]
  }
}

object JettyModule {
  /** Dummy-Service, falls JettyPlugin ohne weitere Jersey-Services gestartet werden sollte.
   * Wenn kein Service mit @Path im Injector ist, bricht Jersey mit folgender Exception ab:
   * com.sun.jersey.api.container.ContainerException: The ResourceConfig instance does not contain any root resource classes.
   */
  @Path("/unused-dummy-path")
  @Singleton
  final class DummyService {
    @GET def get = Response.status(Response.Status.NOT_FOUND).build()
  }
}

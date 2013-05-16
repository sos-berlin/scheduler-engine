package com.sos.scheduler.engine.plugins.jetty.configuration

import Config._
import JettyModule._
import com.fasterxml.jackson.core.Version
import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.databind.module.SimpleModule
import com.fasterxml.jackson.module.scala.DefaultScalaModule
import com.sos.scheduler.engine.plugins.jetty.bodywriters.BodyWriters
import com.sos.scheduler.engine.plugins.jetty.cpp.CppServlet
import com.sos.scheduler.engine.plugins.jetty.log.{MainLogServlet, OrderLogServlet, JobLogServlet}
import com.sos.scheduler.engine.plugins.jetty.services.RootService
import com.sun.jersey.guice.JerseyServletModule
import com.sun.jersey.guice.spi.container.servlet.GuiceContainer

final class JettyModule extends JerseyServletModule {
  override def configureServlets() {
    serve(enginePrefixPath+"/*").`with`(classOf[GuiceContainer]) // Route all requests through GuiceContainer
    serveRegex(enginePrefixPath+"/log").`with`(classOf[MainLogServlet])
    serveRegex(enginePrefixPath+"/"+JobLogServlet.PathInfoRegex).`with`(classOf[JobLogServlet])
    serveRegex(enginePrefixPath+"/"+OrderLogServlet.PathInfoRegex).`with`(classOf[OrderLogServlet])
    for (c <- BodyWriters.messageBodyWriters) bind(c)
    bind(classOf[ObjectMapperJacksonJsonProvider]).toInstance(new ObjectMapperJacksonJsonProvider(newObjectMapper()))
    bind(classOf[RootService])
    serve(cppPrefixPath).`with`(classOf[CppServlet])
    serve(cppPrefixPath+"/*").`with`(classOf[CppServlet])
  }
}

object JettyModule {
  private def newObjectMapper() = {
    def newJacksonModule() = new SimpleModule(classOf[Config].getName, new Version(0, 0, 0, "", null, null))
    val result = new ObjectMapper
    result.registerModule(newJacksonModule())
    result.registerModule(DefaultScalaModule)
    result
  }
}


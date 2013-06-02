package com.sos.scheduler.engine.plugins.jetty

import java.io.File
import java.net.URL
import org.eclipse.jetty.security.LoginService

final case class ServerConfiguration(
  contextPathOption: Option[String],
  portOption: Option[Int] = None,
  webXMLFileOption: Option[File] = None,
  jettyXMLURLOption: Option[URL] = None,
  accessLogFileOption: Option[File] = None,
  loginServiceOption: Option[LoginService] = None,
  resourceBaseURLOption: Option[URL] = None
)
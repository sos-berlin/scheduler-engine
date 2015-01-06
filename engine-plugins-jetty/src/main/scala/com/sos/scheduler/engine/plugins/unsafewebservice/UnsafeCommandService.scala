package com.sos.scheduler.engine.plugins.unsafewebservice

import com.sos.scheduler.engine.kernel.scheduler.SchedulerXmlCommandExecutor
import com.sos.scheduler.engine.plugins.webservice.services.CommandService.executeCommandWithSecurityLevel
import javax.inject.{Inject, Singleton}
import javax.servlet.http.HttpServletRequest
import javax.ws.rs._
import javax.ws.rs.core.{Context, MediaType}

/**
 * Web service breaking the HTTP recommendation for GET to not modify anything - UNSAFE, FACILITATES CSRF.
 * <p>
 * It is especially unsafe in a network with browsers that have access to malicious web pages and JobScheduler.
 * <p>
 * <a href="http://www.w3.org/Protocols/rfc2616/rfc2616-sec9.html#sec9.1.1">RFC 2616, section 9.1.1</a>:
 * "In particular, the convention has been established that the GET and HEAD methods SHOULD NOT have the significance of taking an action other than retrieval."
 * <p>
 * See also <a href="https://www.owasp.org/index.php/CSRF">Cross-Site Request Forgery</a>.
 */
@Path("UNSAFE/command")
@Singleton
final class UnsafeCommandService @Inject private(xmlCommandExecutor: SchedulerXmlCommandExecutor) {

  @GET
  @Produces(Array(MediaType.TEXT_XML))
  def get(
      @DefaultValue("") @QueryParam("command") command: String,
      @Context request: HttpServletRequest) =
    executeCommandWithSecurityLevel(xmlCommandExecutor, command, request)
}

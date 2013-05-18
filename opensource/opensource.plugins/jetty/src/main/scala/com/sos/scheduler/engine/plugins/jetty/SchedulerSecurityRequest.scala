package com.sos.scheduler.engine.plugins.jetty

import javax.servlet.http.HttpServletRequest
import com.sos.scheduler.engine.kernel.security.SchedulerSecurityLevel

object SchedulerSecurityRequest {
  val securityLevelRolePrefix = "seclevel."

  def securityLevel(request: HttpServletRequest): SchedulerSecurityLevel =
    if (requestIsAnonymous(request))
      SchedulerSecurityLevel.all
    else
      SchedulerSecurityLevel.values find { o => request isUserInRole securityLevelToRole(o) } getOrElse SchedulerSecurityLevel.info

  private def requestIsAnonymous(request: HttpServletRequest): Boolean =
    request.getAuthType == null

  private def securityLevelToRole(o: SchedulerSecurityLevel) =
    s"$securityLevelRolePrefix${o.cppName}"
}

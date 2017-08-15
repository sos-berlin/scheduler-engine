package com.sos.scheduler.engine.common.process.windows

/**
  * @author Joacim Zschimmer
  */
final case class Logon(
  credentials: WindowsProcessCredentials,
  withUserProfile: Boolean)
{
  def user = credentials.user
}

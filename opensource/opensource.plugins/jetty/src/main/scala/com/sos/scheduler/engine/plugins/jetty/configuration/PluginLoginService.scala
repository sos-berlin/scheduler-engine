package com.sos.scheduler.engine.plugins.jetty.configuration

import com.sos.scheduler.engine.common.scalautil.SideEffect._
import com.sos.scheduler.engine.plugins.jetty.configuration.PluginLoginService._
import org.eclipse.jetty.security.MappedLoginService
import org.eclipse.jetty.util.security.Credential
import scala.collection.immutable

final class PluginLoginService private extends MappedLoginService {

  setName(RealmName)

  def loadUser(username: String) = null

  def loadUsers() {}
}

object PluginLoginService {
  private val RealmName = "JobScheduler realm"

  def apply(logins: TraversableOnce[Login]): PluginLoginService =
    new PluginLoginService sideEffect { service ⇒
      for (login <- logins) {
        service.putUser(login.name, login.credential, login.roles.toArray)
      }
    }

  final case class Login(name: String, credential: Credential, roles: immutable.Seq[String])
}

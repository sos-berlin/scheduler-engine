package com.sos.scheduler.engine.plugins.jetty

import scala.collection.JavaConversions._
import com.sos.scheduler.engine.util.xml.NamedChildElements
import org.eclipse.jetty.security.MappedLoginService
import org.eclipse.jetty.util.security.{Credential, Password}
import org.w3c.dom.Element

import PluginLoginService._
import com.google.common.base.Splitter
import java.util.regex.Pattern

class PluginLoginService(logins: Iterable[Login]) extends MappedLoginService {
  logins foreach { o => putUser(o.name, o.credential, o.roles) }
  setName(realmName)

  def loadUser(username: String) = null

  def loadUsers() {}
}

object PluginLoginService {
  val realmName = "JobScheduler realm"

  def apply(element: Element) = new PluginLoginService(logins(element))

  private val spaceSplitter = Splitter.on(Pattern.compile(" +")).omitEmptyStrings();

  private def logins(element: Element) =
    new NamedChildElements("logins", element) flatMap {
      new NamedChildElements("login", _) map { e =>
        Login(
          e.getAttribute("name"),
          new Password(e.getAttribute("password")),
          spaceSplitter.split(e.getAttribute("roles")).toArray)
      }
    }

  case class Login(name: String, credential: Credential, roles: Array[String])
}

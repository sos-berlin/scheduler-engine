package com.sos.scheduler.engine.agent.web.test

import com.google.common.base.Splitter
import com.sos.scheduler.engine.agent.web.test.ShiroTest._
import com.sos.scheduler.engine.common.scalautil.SideEffect.ImplicitSideEffect
import com.sos.scheduler.engine.common.utils.JavaResource
import java.util.Objects.requireNonNull
import org.apache.shiro.authc.{AuthenticationToken, IncorrectCredentialsException, UsernamePasswordToken}
import org.apache.shiro.authz.UnauthorizedException
import org.apache.shiro.config.Ini.Section
import org.apache.shiro.config.{Ini, IniSecurityManagerFactory}
import org.apache.shiro.io.ResourceUtils.CLASSPATH_PREFIX
import org.apache.shiro.subject.Subject
import org.apache.shiro.subject.support.DefaultSubjectContext
import org.apache.shiro.{SecurityUtils, UnavailableSecurityManagerException}
import org.scalatest.FreeSpec
import scala.collection.JavaConversions._

/**
  * @author Joacim Zschimmer
  */
final class ShiroTest extends FreeSpec {

  "Valid login" in {
    val subject = new Sec().newSubject()
    assert(!subject.isAuthenticated)
    subject.login(new UsernamePasswordToken("aGui", "TEST-PASSWORD"))
    assert(subject.isAuthenticated)
  }

  "Valid access token" in {
    val subject = new Sec().login(AccessToken("MASTER-SECRET-ACCESS-TOKEN"))
    assert(subject.isAuthenticated)
    assert(subject.getPrincipal == "someMaster")
  }

  "Invalid access token" in {
    val sec = new Sec
    intercept[InvalidAccessTokenException] {
      sec.login(AccessToken("INVALID-ACCESS-TOKEN"))
    }
  }

  "Rights of someMaster" in {
    val subject = new Sec().login(AccessToken("MASTER-SECRET-ACCESS-TOKEN"))
    assert(subject.hasRole("master"))
    subject.checkPermission("WebService:POST:api:tunnel:42")
    subject.checkPermissions("WebService:POST:api:command", "Command:StartApiTask")
    intercept[UnauthorizedException] { subject.checkPermissions("WebService:POST:api:command", "command:Terminate") }
  }

  "Rights of GUI" in {
    val sec = new Sec
    val subject = sec.newSubject()
    subject.login(new UsernamePasswordToken("aGui", "TEST-PASSWORD"))
    assert(subject.hasRole("viewer"))
    intercept[UnauthorizedException] { subject.checkPermission("WebService:POST:api:tunnel:42") }
    subject.checkPermissions("WebService:POST:api:command", "Command:Terminate")
    intercept[UnauthorizedException] { subject.checkPermissions("WebService:POST:api:command", "Command:Evil") }
    intercept[UnauthorizedException] { subject.checkPermissions("WebService:POST:api:tunnel") }
    subject.checkPermission("WebService:GET:api:tasks")
  }

  "Rights of a monitor" in {
    val sec = new Sec
    val subject = sec.newSubject()
    subject.login(new UsernamePasswordToken("nagios", "TEST-PASSWORD"))
    assert(subject.hasRole("monitor"))
    intercept[UnauthorizedException] { subject.checkPermission("WebService:POST:api:tunnel:42") }
    intercept[UnauthorizedException] { subject.checkPermission("WebService:POST:api:command") }
    intercept[UnauthorizedException] { subject.checkPermission("WebService:GET:api:tasks") }
    subject.checkPermission("WebService:GET:api:isAlive")
  }

  "SecurityUtils.getSubject is not available" in {
    intercept[UnavailableSecurityManagerException] { SecurityUtils.getSubject }
  }

  "Invalid login" in {
    val sec = new Sec
    val subject = sec.newSubject()
    intercept[IncorrectCredentialsException] {
      subject.login(new UsernamePasswordToken("someMaster", "INVALID-PASSWORD"))
    }
    assert(!subject.isAuthenticated)
  }
}

private object ShiroTest {
  private val ShiroIniResource = JavaResource("com/sos/scheduler/engine/agent/web/test/shiro.ini")

  private class Sec {
    private val ini = new Ini sideEffect { _.loadFromPath(CLASSPATH_PREFIX + ShiroIniResource.path) }
    private val securityManager = new IniSecurityManagerFactory(ini).getInstance

    def login(accessToken: AccessToken): Subject = {
      val subject = newSubject()
      val authenticationToken = accessTokenToAuthenticationToken(accessToken) getOrElse { throw new InvalidAccessTokenException }
      subject.login(authenticationToken)
      subject
    }

    def newSubject(): Subject = securityManager.createSubject(new DefaultSubjectContext())

    def accessTokenToAuthenticationToken(accessToken: AccessToken): Option[AuthenticationToken] =
      for (userName ← accessTokenToUserName(accessToken);
           auth ← userNameToAuthenticationTokenOption(userName))
        yield auth

    private object accessTokenToUserName {
      private val accessTokensSectionOption: Option[Section] = Option(ini.get("accessTokens"))

      def apply(accessToken: AccessToken): Option[String] =
        for (accessTokensSection ← accessTokensSectionOption;
             userName ← Option(accessTokensSection.get(accessToken.string)))
          yield userName
    }

    private object userNameToAuthenticationTokenOption {
      private val usersSection: Section = requireNonNull(ini.get("users"), "Missing section [users] in shiro.ini")
      private val passwordSplitter = Splitter.on("""\s*,\s*""".r.pattern).trimResults

      def apply(name: String): Option[UsernamePasswordToken] =
        for (value ← Option(usersSection.get(name));
             password ← passwordSplitter.split(value).headOption)
        yield new UsernamePasswordToken(name, password)
    }
  }

  private case class AccessToken(string: String)

  private class InvalidAccessTokenException extends RuntimeException
}

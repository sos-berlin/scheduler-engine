package com.sos.scheduler.engine.kernel.security

import com.sos.jobscheduler.base.generic.SecretString
import com.sos.jobscheduler.common.auth.SecretStringGenerator.newSecretString
import com.sos.jobscheduler.common.auth.UserId
import javax.inject.{Inject, Singleton}
import scala.collection.JavaConversions._

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class AccessTokenRegister @Inject()() {

  private val register = new java.util.concurrent.ConcurrentHashMap[SecretString, UserId] withDefault {
    _ ⇒ throw new NoSuchElementException("Invalid access token")
  }

  private[security] def size = register.size

  def newAccessTokenForUser(userId: UserId): SecretString = {
    val accessToken = newSecretString()
    register += accessToken → userId
    accessToken
  }

  def remove(accessToken: SecretString): Unit = {
    register -= accessToken
  }

  val validate: PartialFunction[SecretString, UserId] = register

  override def toString = s"AccessTokenRegister(${register.size} access tokens)"
}

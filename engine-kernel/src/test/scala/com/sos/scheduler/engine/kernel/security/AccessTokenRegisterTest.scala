package com.sos.scheduler.engine.kernel.security

import com.sos.jobscheduler.base.generic.SecretString
import com.sos.jobscheduler.common.auth.UserId
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class AccessTokenRegisterTest extends FreeSpec {

  private val register = new AccessTokenRegister

  "AccessTokeRegister" in {
    assert(register.size == 0)
    val a = register.newAccessTokenForUser(UserId("A"))
    val b = register.newAccessTokenForUser(UserId("B"))
    assert(a != b)
    assert(register.size == 2)
    assert(register.validate.lift(a) == Some(UserId("A")))
    assert(register.validate.lift(b) == Some(UserId("B")))
    assert(register.validate.lift(SecretString("UNKNOWN")) == None)

    register.remove(a)
    assert(register.validate.lift(a) == None)

    register.remove(b)
    assert(register.validate.lift(b) == None)
    assert(register.size == 0)
  }
}

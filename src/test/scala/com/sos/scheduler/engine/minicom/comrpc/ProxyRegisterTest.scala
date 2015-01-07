package com.sos.scheduler.engine.minicom.comrpc

import com.google.inject.Guice
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.minicom.comrpc.ProxyRegister.{DuplicateKeyException, ProxyIUnknown}
import com.sos.scheduler.engine.minicom.comrpc.calls.ProxyId
import com.sos.scheduler.engine.minicom.types.IUnknown
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class ProxyRegisterTest extends FreeSpec {

  private val proxyRegister = Guice.createInjector().apply[ProxyRegister]
  private val externalProxyId = ProxyId(0x123456789abcdefL)

  "External ProxyId.Null" in {
    proxyRegister.registerProxyId(ProxyId.Null, "") shouldEqual None
  }

  "External ProxyId" in {
    val Some(iUnknown) = proxyRegister.registerProxyId(externalProxyId, "")
    iUnknown.asInstanceOf[ProxyIUnknown].id shouldEqual externalProxyId
    intercept[DuplicateKeyException] { proxyRegister.registerProxyId(externalProxyId, "") }
    proxyRegister.iUnknownToProxyId(iUnknown) shouldEqual (externalProxyId, false)
    proxyRegister.size shouldEqual 1
  }

  "Own IUnknown" in {
    proxyRegister.size shouldEqual 1
    val iUnknown = new IUnknown {}
    val (proxyId, true) = proxyRegister.iUnknownToProxyId(iUnknown)
    proxyId.index shouldEqual 1
    proxyRegister.iUnknownToProxyId(iUnknown) shouldEqual (proxyId, false)
    intercept[DuplicateKeyException] { proxyRegister.registerProxyId(proxyId, "") }
    proxyRegister.size shouldEqual 2
    val (otherProxyId, true) = proxyRegister.iUnknownToProxyId(new IUnknown {})
    otherProxyId.index shouldEqual 2
    proxyRegister.size shouldEqual 3
  }

  "removeProxy" in {
    proxyRegister.size shouldEqual 3
    proxyRegister.removeProxy(externalProxyId)
    proxyRegister.size shouldEqual 2
    proxyRegister.removeProxy(externalProxyId)
    proxyRegister.size shouldEqual 2
  }

  "remoteProxy closes AutoCloseable" in {
    class A extends IUnknown with AutoCloseable {
      var isClosed = false
      def close() = {
        isClosed = true
        throw new Exception("Should be ignored, only logged")
      }
    }
    val a = new A
    val (proxyId, true) = proxyRegister.iUnknownToProxyId(a)
    assert(!a.isClosed)
    proxyRegister.removeProxy(proxyId)
    assert(a.isClosed)
  }
}

package com.sos.scheduler.engine.minicom.comrpc

import com.google.inject.Guice
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.minicom.comrpc.ProxyRegister.DuplicateKeyException
import com.sos.scheduler.engine.minicom.comrpc.calls.ProxyId
import com.sos.scheduler.engine.minicom.types.{IDispatch, IDispatchable}
import org.junit.runner.RunWith
import org.mockito.Mockito._
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import org.scalatest.mock.MockitoSugar.mock

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class ProxyRegisterTest extends FreeSpec {

  private val proxyRegister = Guice.createInjector().apply[ProxyRegister]
  private val externalProxyId = ProxyId(0x123456789abcdefL)

  "External ProxyId" in {
    val proxy = newProxy(externalProxyId)
    proxyRegister.registerProxy(proxy)
    proxy.id shouldEqual externalProxyId
    proxyRegister.iDispatchToProxyId(proxy) shouldEqual (externalProxyId, false)
    proxyRegister.iDispatchable(externalProxyId) shouldEqual proxy
    proxyRegister.size shouldEqual 1
    intercept[DuplicateKeyException] { proxyRegister.registerProxy(newProxy(externalProxyId)) }
  }

  "Own IDispatch" in {
    proxyRegister.size shouldEqual 1
    val iDispatch = mock[IDispatchable]
    val (proxyId, true) = proxyRegister.iDispatchToProxyId(iDispatch)
    proxyId.index shouldEqual 1
    proxyRegister.iDispatchToProxyId(iDispatch) shouldEqual (proxyId, false)
    proxyRegister.iDispatchable(proxyId) shouldEqual iDispatch
    intercept[DuplicateKeyException] { proxyRegister.registerProxy(newProxy(proxyId)) }

    proxyRegister.size shouldEqual 2
    val (otherProxyId, true) = proxyRegister.iDispatchToProxyId(mock[IDispatchable])
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
    trait A extends IDispatch with AutoCloseable
    val a = mock[A]
    when (a.close()) thenThrow new Exception("SHOULD BE IGNORED, ONLY LOGGED")
    val (proxyId, true) = proxyRegister.iDispatchToProxyId(a)
    proxyRegister.removeProxy(proxyId)
    verify(a).close()
  }

  private def newProxy(proxyId: ProxyId, name: String = "") =
    new ProxyIDispatch.Simple(mock[Remoting], proxyId, name)
}

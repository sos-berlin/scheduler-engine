package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.minicom.comrpc.CallDeserializer._
import com.sos.scheduler.engine.minicom.comrpc.calls.{Call, CallCall, CreateInstanceCall, GetIDsOfNamesCall, InvokeCall, ObjectCall, ProxyId, QueryInterfaceCall, ReleaseCall}
import com.sos.scheduler.engine.minicom.types.{CLSID, DISPID, IID}
import java.nio.ByteBuffer
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
private[comrpc] final class CallDeserializer(protected val proxyRegister: ProxyRegister, protected val buffer: ByteBuffer)
extends IUnknownDeserializer {

  def readCall(): Call =
    readByte() match {
      case MessageClass.Session ⇒ readSessionCall()
      case MessageClass.Object ⇒ readObjectCall()
    }

  private def readSessionCall(): Call = {
    val sessionId = readInt64()  // ???
    readByte() match {
      case MessageCommand.CreateInstance ⇒
        val clsid = CLSID(readUUID())
        val outer = readIUnknownOption()
        val context = readInt32()
        val n = readInt32()
        val iids = immutable.Seq.fill(n) { IID(readUUID()) }
        CreateInstanceCall(clsid, outer, context, iids)
    }
  }

  private def readObjectCall(): ObjectCall = {
    val proxyId = ProxyId(readInt64())
    readByte() match {

      case MessageCommand.Release ⇒
        ReleaseCall(proxyId)

      case MessageCommand.QueryInterface ⇒
        val iid = IID(readUUID())
        QueryInterfaceCall(proxyId, iid)

      case MessageCommand.GetIDsOfNames ⇒
        val iid = IID(readUUID())
        val localeId = readInt32()
        val names = immutable.Seq.fill(readInt32()) { readString() }
        GetIDsOfNamesCall(proxyId, iid, localeId, names)

      case MessageCommand.Invoke ⇒
        val dispatchId = DISPID(readInt32())
        val iid = IID(readUUID())
        val localeId = readInt32()
        require(localeId == 0)
        val flags = readInt32()
        val argumentCount = readInt32()
        val namedArgumentCount = readInt32()
        require(namedArgumentCount == 0)
        val arguments = readArguments(argumentCount)
        InvokeCall(proxyId, dispatchId, iid, flags, arguments, namedArguments = Nil)

      case MessageCommand.Call ⇒
        val methodName = readString()
        val argumentCount = readInt32()
        val namedArgumentCount = readInt32()
        require(namedArgumentCount == 0)
        logger.trace(s"Call '$methodName' with $argumentCount arguments")
        val arguments = readArguments(argumentCount)
        CallCall(proxyId, methodName, arguments)
    }
  }

  private def readArguments(n: Int): immutable.Seq[Any] = immutable.Seq.fill(n) { readVariant() } .reverse
}

object CallDeserializer {
  private val logger = Logger(getClass)

  object MessageCommand {
    val CreateInstance  = 'C'.toByte
    val Release         = 'R'.toByte
    val QueryInterface  = 'Q'.toByte
    val GetIDsOfNames   = 'G'.toByte
    val Invoke          = 'I'.toByte
    val Call            = 'A'.toByte
  }

  def deserializeCall(proxyRegister: ProxyRegister, buffer: ByteBuffer) = new CallDeserializer(proxyRegister, buffer).readCall()
}

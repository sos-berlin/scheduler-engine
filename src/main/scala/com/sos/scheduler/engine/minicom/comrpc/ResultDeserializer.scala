package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.comrpc.ResultDeserializer._
import com.sos.scheduler.engine.minicom.comrpc.calls.{CreateInstanceResult, GetIDsOfNamesResult, InvokeResult}
import com.sos.scheduler.engine.minicom.types.HRESULT._
import com.sos.scheduler.engine.minicom.types.{COMException, DISPID, HRESULT}
import java.nio.ByteBuffer
import org.scalactic.Requirements._
import scala.collection.mutable

/**
 * @author Joacim Zschimmer
 */
final class ResultDeserializer(
  protected val connection: MessageConnection,
  protected val proxyRegister: ProxyRegister,
  protected val buffer: ByteBuffer)
extends IUnknownDeserializer {

  def readCreateInstanceResult(): CreateInstanceResult = {
    readAnswerHeader()
    require(HRESULT(readInt32()) == S_OK)
    CreateInstanceResult(readIDispatchableOption().get)
  }

  def readGetIDsOfNamesResult(): GetIDsOfNamesResult = {
    readAnswerHeader()
    val dispids = Vector.fill(readInt32()) { DISPID(readInt32()) }
    GetIDsOfNamesResult(dispids)
  }

  def readInvokeResult(): InvokeResult = {
    readAnswerHeader()
    InvokeResult(readVariant())
  }

  private def readAnswerHeader() {
    readByte() match {
      case MessageClass.Answer ⇒
        val hr = HRESULT(readInt32())
        require(!hr.isError)
      case MessageClass.Error ⇒
        val strings = mutable.Buffer[String]()
        for (_ ← 1 to 3) readString() match {
          case KeyValueRegex("name", v) ⇒ strings += v
          case KeyValueRegex("code", v) ⇒ strings += v
          case KeyValueRegex("what", v) ⇒ strings += v
          case v ⇒ strings += v
        }
        throw new COMException(DISP_E_EXCEPTION, strings mkString " ")
    }
  }
}

object ResultDeserializer {
  private val KeyValueRegex = "([a-z]+)=(.*)".r
  private val ComErrorRegex = "COM-[0-9a-fA-F]{8}".r
}

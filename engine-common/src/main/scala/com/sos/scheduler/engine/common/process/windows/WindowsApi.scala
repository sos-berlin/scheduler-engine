package com.sos.scheduler.engine.common.process.windows

import com.sos.scheduler.engine.common.process.windows.MyUserenv.PROFILEINFO
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.system.OperatingSystem.isWindows
import com.sun.jna.platform.win32.Kernel32Util.formatMessageFromLastErrorCode
import com.sun.jna.platform.win32.WinBase.{WAIT_FAILED, WAIT_OBJECT_0}
import com.sun.jna.platform.win32.WinDef._
import com.sun.jna.platform.win32.WinError.WAIT_TIMEOUT
import com.sun.jna.platform.win32.WinNT.{HANDLE, HANDLEByReference}
import com.sun.jna.platform.win32.{Advapi32, Kernel32}
import com.sun.jna.ptr.{IntByReference, PointerByReference}
import com.sun.jna.win32.StdCallLibrary
import com.sun.jna.win32.W32APIOptions.UNICODE_OPTIONS
import com.sun.jna.{Memory, Native, Pointer}
import java.nio.file.{Path, Paths}
import scala.annotation.tailrec

/**
  * @author Joacim Zschimmer
  */
object WindowsApi {
  private val logger = Logger(getClass)
  val MAX_PATH = 260
  private[windows] lazy val kernel32 = Native.loadLibrary("kernel32", classOf[Kernel32], UNICODE_OPTIONS)
  private[windows] lazy val advapi32 = Native.loadLibrary("advapi32", classOf[Advapi32], UNICODE_OPTIONS)
  private[windows] lazy val myUserenv = Native.loadLibrary("userenv", classOf[MyUserenv], UNICODE_OPTIONS)
  private[windows] lazy val myKernel32 = Native.loadLibrary("kernel32", classOf[MyKernel32], UNICODE_OPTIONS)
  private[windows] lazy val myAdvapi32 = Native.loadLibrary("advapi32", classOf[MyAdvapi32], UNICODE_OPTIONS)

  private[windows] def waitForSingleObject(handle: HANDLE, timeout: Int): Boolean = {
    requireWindows("WaitForSingleObject")
    kernel32.WaitForSingleObject(handle, timeout) match {
      case WAIT_OBJECT_0 ⇒ true
      case WAIT_TIMEOUT ⇒ false
      case WAIT_FAILED ⇒ throwLastError("WaitForSingleObject")
      case o ⇒ throw new WindowsException(s"WaitForSingleObject returned $o")
    }
  }

  def processHandleCount: Int = {
    val ref = new IntByReference
    call("GetProcessHandleCount") {
      myKernel32.GetProcessHandleCount(kernel32.GetCurrentProcess(), ref)
    }
    ref.getValue
  }

  def tempPath: Path = {
    requireWindows("GetTempPath")
    val a = new Array[Char](MAX_PATH + 1)
    val length = kernel32.GetTempPath(new DWORD(a.length), a).intValue
    if (length <= 0 || length > a.length) throw new WindowsException("GetTempPath failed")
    Paths.get(new String(a, 0, length))
  }

  /**
    * Returns something like """C:\Windows""".
    */
  def windowsDirectory: Path = {
    val a = new Array[Char](MAX_PATH + 1)
    val length = myKernel32.GetSystemWindowsDirectory(a, a.length)
    if (length <= 0 || length > a.length) throw new WindowsException("GetSystemWindowsDirectory failed")
    Paths.get(new String(a, 0, length))
  }

  private[windows] def openProcessToken(process: HANDLE, desiredAccess: Int): HANDLE =
    handleCall("OpenProcessToken")(myAdvapi32.OpenProcessToken(process, desiredAccess, _))

  /**
    * Calls `CreateEnvironmentBlock`, returns the environment defined for the given user.
    */
  private[windows] def usersEnvironment(userToken: HANDLE): Map[String, String] = {
    val strings = {
      val handle = {
        val ref = new PointerByReference
        call("CreateEnvironmentBlock") {
          myUserenv.CreateEnvironmentBlock(ref, userToken, false)
        }
        ref.getValue
      }
      try {
        val builder = Vector.newBuilder[String]
        @tailrec def continue(offset: Int): Unit =
          handle.getWideString(offset) match {
            case "" ⇒
            case string ⇒
              builder += string
              continue(offset + 2 * (string.length + 1))
          }
        continue(0)
        builder.result
      } finally {
        call("DestroyEnvironmentBlock") {
          myUserenv.DestroyEnvironmentBlock(handle)
        }
      }
    }

    (strings map { o ⇒
      o indexOf '=' match {
        case -1 ⇒ o → ""
        case i ⇒ o.substring(0, i) -> o.substring(i + 1)
      }
    })
    .toMap
  }

  private[windows] def stringToMemory(string: String): Memory = {
    val m = new Memory(2 * (string.length + 1))
    m.setWideString(0, string)
    m
  }

  private[windows] def handleCall(name: String)(apiFunction: HANDLEByReference ⇒ Boolean): HANDLE = {
    val ref = new HANDLEByReference
    call(name)(apiFunction(ref))
    ref.getValue
  }

  private[windows] def call(functionName: String, args: String*)(apiFunction: ⇒ Boolean): Unit = {
    requireWindows(functionName)
    logger.trace(s"Calling Windows API: $functionName ${args mkString ", "}")
    val ok = apiFunction
    if (!ok)
      throwLastError(functionName)
  }

  def throwLastError(function: String): Nothing = {
    requireWindows("GetLastError")
    val err = kernel32.GetLastError
    throw new WindowsException(f"WINDOWS-${messageIdToString(err)} ($function) ${formatMessageFromLastErrorCode(err)}")
  }

  def messageIdToString(id: Int) =
    if (id <= 0xc0ffffff/*negative*/) f"0x$id%08x" else id.toString

  //@Deprecated  // Does not work for 0xc0000142 STATUS_DLL_INIT_FAILED
  //private[windows] def messageToString(id: Int) =
  //  s"${messageIdToString(id)} ${formatMessage(id)}"
  //
  ///** Returns "" on error. */
  //@Deprecated  // Does not work for 0xc0000142 STATUS_DLL_INIT_FAILED
  //private def formatMessage(messageId: Int): String = {
  //  val normalizedId = W32Errors.HRESULT_FROM_WIN32(messageId).intValue
  //  val buffer = new PointerByReference
  //  val length = Kernel32.INSTANCE.FormatMessage(
  //    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
  //    null, normalizedId, 0/*MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT*/,
  //    buffer, 0, null)
  //  if (length == 0)
  //    ""
  //  else
  //    try buffer.getValue.getWideString(0).trim
  //    finally freeLocalMemory(buffer.getValue)
  //}

  private def requireWindows(functionName: String) = {
    if (!isWindows) sys.error(s"Windows API '$functionName' is only available under Microsoft Windows")
  }

  private[windows] final class WindowsException(message: String) extends RuntimeException(message)

  private[windows] trait MyKernel32 extends StdCallLibrary {
    def GetProcessHandleCount(hProcess: HANDLE, handleCount: IntByReference): Boolean
    def GetSystemWindowsDirectory(buffer: Array[Char], nBufferLength: Int): Int
  }

  private[windows] trait MyAdvapi32 extends StdCallLibrary {
    def CredRead(targentName: String, typ: Int, flags: Int, credentialRef: PointerByReference): Boolean
    def CredFree(credentials: Pointer): Unit
    def OpenProcessToken(process: HANDLE, desiredAccess: Int, result: HANDLEByReference): Boolean
  }

  private[windows] trait MyUserenv extends StdCallLibrary {
    def LoadUserProfile(userToken: HANDLE, profileInfo: PROFILEINFO): Boolean
    def UnloadUserProfile(userToken: HANDLE, profileHandle: HANDLE): Boolean
    def CreateEnvironmentBlock(environment: PointerByReference, userToken: HANDLE, inherit: Boolean): Boolean
    def DestroyEnvironmentBlock(environment: Pointer): Boolean
  }
}

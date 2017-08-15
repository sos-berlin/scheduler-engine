package com.sos.scheduler.engine.common.process.windows

import com.sos.scheduler.engine.common.process.windows.WindowsApi._
import com.sun.jna.platform.win32.Kernel32Util.closeHandle
import com.sun.jna.platform.win32.WinBase.{HANDLE_FLAG_INHERIT, INVALID_HANDLE_VALUE, SECURITY_ATTRIBUTES}
import com.sun.jna.platform.win32.WinNT.{CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, FILE_SHARE_READ, GENERIC_WRITE, HANDLE, HANDLEByReference}
import java.io.File
import java.util.concurrent.atomic.AtomicBoolean

/**
  * @author Joacim Zschimmer
  */
private[windows] class Redirection(
  val startupInfoHandle: HANDLE, closeStartupInfoHandle: Boolean,
  val pipeHandle: HANDLE)
{
  private val released = new AtomicBoolean
  private val finished = new AtomicBoolean
  private val pipeClosed = new AtomicBoolean

  def releaseStartupInfoHandle(): Unit = {
    if (!released.getAndSet(true)) {
      if (closeStartupInfoHandle) {
        closeHandle(startupInfoHandle)
      }
    }
  }

  /**
    * Closes the pipe (if opened) and await the completion of PipeToFileThread (if started).
    */
  def finish(): Unit = {
    if (!finished.getAndSet(true)) {
      closePipe()
    }
  }

  def closePipe(): Unit = {
    if (!pipeClosed.getAndSet(true) && pipeHandle != INVALID_HANDLE_VALUE) {
      closeHandle(pipeHandle)
    }
  }
}

private object Redirection {
  private val BufferSize = 4096

  def forStdinPipe: Redirection = {
    val readRef = new HANDLEByReference
    val writeRef = new HANDLEByReference
    val security = new SECURITY_ATTRIBUTES
    security.bInheritHandle = true
    call("CreatePipe") {
      kernel32.CreatePipe(readRef, writeRef, security, BufferSize)
    }
    call("SetHandleInformation") {
      kernel32.SetHandleInformation(writeRef.getValue, HANDLE_FLAG_INHERIT, 0)
    }
    new Redirection(readRef.getValue, true, writeRef.getValue)
  }

  def forDirectFile(file: File): Redirection = {
    val security = new SECURITY_ATTRIBUTES
    security.bInheritHandle = true
    val handle = kernel32.CreateFile(
      file.toString,
      GENERIC_WRITE,
      FILE_SHARE_READ,
      security,
      CREATE_ALWAYS,
      FILE_ATTRIBUTE_TEMPORARY,
      null)
    if (handle == INVALID_HANDLE_VALUE) throwLastError(s"CreateFile '$file'")
    new Redirection(handle, true, INVALID_HANDLE_VALUE)
  }
}


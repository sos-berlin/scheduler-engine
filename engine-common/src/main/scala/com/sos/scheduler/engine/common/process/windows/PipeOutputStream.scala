package com.sos.scheduler.engine.common.process.windows

import com.sos.scheduler.engine.common.process.windows.WindowsApi.{call, kernel32}
import com.sun.jna.platform.win32.WinNT.HANDLE
import com.sun.jna.ptr.IntByReference
import java.io.OutputStream

/**
  * @author Joacim Zschimmer
  */
private[windows] abstract class PipeOutputStream(pipeHandle: HANDLE) extends OutputStream {

  private val written = new IntByReference

  def write(b: Int) = write(Array(b.toByte), 0, 1)

  override def write(bytes: Array[Byte], offset: Int, length: Int) = {
    val a = if (offset == 0 && length == bytes.length) bytes else bytes.slice(offset, offset + length)
    require(length <= a.length, "Invalid length")
    call("WriteFile") {
      kernel32.WriteFile(pipeHandle, bytes, length, written, null)
    }
    if (written.getValue != length) sys.error(s"Less bytes written (${written.getValue} to process' stdin than expected ($length)")
  }

  //override def flush() =
  //  call("FlushFileBuffers") {
  //    kernel32.FlushFileBuffers(pipeHandle)  // Error "WINDOWS-109 Die Pipe wurde beendet" in JS861IT
  //  }
}

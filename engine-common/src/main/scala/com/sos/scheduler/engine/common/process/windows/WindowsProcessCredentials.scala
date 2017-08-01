package com.sos.scheduler.engine.common.process.windows

import com.sos.scheduler.engine.base.generic.SecretString
import com.sos.scheduler.engine.common.process.windows.WindowsApi.{call, myAdvapi32}
import com.sos.scheduler.engine.common.system.OperatingSystem.isWindows
import com.sun.jna.Structure
import com.sun.jna.ptr.PointerByReference

/**
  * @author Joacim Zschimmer
  */
final case class WindowsProcessCredentials(user: WindowsUserName, password: SecretString)

object WindowsProcessCredentials {

  def byKey(key: String): WindowsProcessCredentials = {
    require(isWindows, "Windows credentials can only be read under Microsoft Windows")
    try {
      val ref = new PointerByReference
      call("CredRead") {
        myAdvapi32.CredRead(key, MyAdvapi32.CRED_TYPE_GENERIC, 0, ref)
      }
      val credential = Structure.newInstance(classOf[MyAdvapi32.CREDENTIAL], ref.getValue).asInstanceOf[MyAdvapi32.CREDENTIAL]
      credential.read()
      val password = SecretString(credential.credentialBlob.getWideString(0))
      val result = WindowsProcessCredentials(WindowsUserName(credential.userName.toString), password)
      myAdvapi32.CredFree(ref.getValue)
      result
    } catch { case e: Exception ⇒
      throw new IllegalArgumentException(s"Windows Credential Manager does not return an entry named '$key': ${e.getMessage}", e)
    }
  }
}

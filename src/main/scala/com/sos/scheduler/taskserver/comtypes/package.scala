package com.sos.scheduler.taskserver

import com.sos.scheduler.taskserver.comrpc.IUnknown
import com.sos.scheduler.taskserver.comrpc.types.{CLSID, IID}

/**
 * @author Joacim Zschimmer
 */
package object comtypes {
  type ClsidToFactory = ((CLSID, IID)) ⇒ (() ⇒ IUnknown)
}

package com.sos.scheduler.engine.kernel.plugin

import scala.collection.{immutable, mutable}

/** Andere Plugins können hier ihre Erweiterungen dieses Plugins registrieren.
 * @author Joacim Zschimmer
 */
trait ExtensionRegister[A] {
  this: Plugin ⇒

  private val register = new mutable.ArrayBuffer[A]
  @volatile private var _extensions: immutable.Seq[A] = null

  def addExtension(extension: A) {
    synchronized {
      if (isActive || _extensions != null) throw new IllegalStateException
      register.append(extension)
    }
  }

  def extensions: immutable.Seq[A] = {
    if (!isPrepared) throw new IllegalStateException()
    synchronized {
      if (_extensions == null) {
        _extensions = register.toVector
      }
    }
    _extensions
  }
}

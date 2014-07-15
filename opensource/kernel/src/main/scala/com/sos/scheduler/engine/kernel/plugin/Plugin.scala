package com.sos.scheduler.engine.kernel.plugin

import com.sos.scheduler.engine.common.scalautil.HasCloser

/**
 * @author Joacim Zschimmer
 */
trait Plugin extends HasCloser {

  private var _isPrepared = false
  private var _isActive = false

  onClose {
    _isActive = false
    _isPrepared = false
  }

  final def prepare() {
    onPrepare()
    _isPrepared = true
  }

  def onPrepare() {}

  final def activate() {
    if (!_isPrepared)  throw new IllegalStateException
    onActivate()
    _isActive = true
  }

  def onActivate() {}

  def xmlState: String = ""

  final def isPrepared = _isPrepared

  final def isActive = _isActive
}

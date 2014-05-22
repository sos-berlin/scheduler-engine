package com.sos.scheduler.engine.kernel.plugin

/**
 * @author Joacim Zschimmer
 */
trait Plugin {

  private var _isPrepared = false
  private var _isActive = false

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

  def close() {
    _isActive = false
    _isPrepared = false
  }

  def xmlState: String = ""

  final def isPrepared = _isPrepared

  final def isActive = _isActive
}

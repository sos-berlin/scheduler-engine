package com.sos.scheduler.engine.kernel.plugin

/**
 * @author Joacim Zschimmer
 */
trait Plugin {

  private var _isPrepared = false
  private var _isActive = false

  def prepare() {
    _isPrepared = true
  }

  def activate() {
    if (!_isPrepared)  throw new IllegalStateException
    _isActive = true
  }

  def close() {
    _isActive = false
    _isPrepared = false
  }

  def xmlState: String = ""

  final def isPrepared = _isPrepared

  final def isActive = _isActive
}

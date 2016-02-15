package com.sos.scheduler.engine.tests.jira.js1103

/**
  * @author Joacim Zschimmer
  */
private[js1103] final class MaximumCounter(initial: Int = 0) extends Mutable {
  private var _number = initial
  private var _maximum = _number

  def +=(o: Int): Unit = {
    _number += o
    if (_maximum < _number) _maximum = _number
  }

  def -=(o: Int) = +=(-o)

  def maximum = _maximum
}

package com.sos.scheduler.engine.kernel.scheduler

import com.google.inject.Injector

/**
  * @author Joacim Zschimmer
  */
trait HasInjector {
  def injector: Injector
}

object HasInjector {
  def apply(injector: Injector): HasInjector = new Standard(injector)

  private class Standard(val injector: Injector) extends HasInjector
}

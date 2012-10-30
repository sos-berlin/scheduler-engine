package com.sos.scheduler.engine.common.scalautil

class ModifiedBy[A](delegate: A) {
  def modifiedBy(f: A => Unit) = {
    f(delegate)
    delegate
  }
}

object ModifiedBy {
  implicit def modifiedBy[A](a: A) = new ModifiedBy(a)
}

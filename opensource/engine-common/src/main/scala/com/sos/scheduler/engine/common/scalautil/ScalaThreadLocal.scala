package com.sos.scheduler.engine.common.scalautil

object ScalaThreadLocal {
  def threadLocal[A](f: => A) = new MyThreadLocal[A] {
    override def initialValue = f
  }

  implicit def get[A](o: MyThreadLocal[A]): A = o.get

  abstract class MyThreadLocal[A] extends ThreadLocal[A]
}
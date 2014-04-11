package com.sos.scheduler.engine.kernel.configuration

import com.google.inject.Scopes.SINGLETON
import com.google.inject.{Provider, AbstractModule}
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.implicitClass
import scala.reflect.ClassTag

abstract class ScalaAbstractModule extends AbstractModule {

  private lazy val myBinder = binder skipSources classOf[ScalaAbstractModule]

  final def bindInstance[A <: AnyRef : ClassTag](instance: A) =
    myBinder bind implicitClass[A] toInstance instance

  final def provideSingleton[A <: AnyRef : ClassTag](provider: => A) =
    provide[A](provider) in SINGLETON

  final def provide[A <: AnyRef : ClassTag](provider: => A) =
    myBinder bind implicitClass[A] toProvider new Provider[A] { def get = provider }
}

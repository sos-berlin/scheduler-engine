package com.sos.scheduler.engine.kernel.configuration

import com.google.inject.Scopes.SINGLETON
import com.google.inject.{Provider, AbstractModule}
import scala.reflect.ClassTag

abstract class ScalaAbstractModule extends AbstractModule {

  final def bindInstance[A <: AnyRef](instance: A)(implicit c: ClassTag[A]) =
    bind(c.runtimeClass.asInstanceOf[Class[A]]) toInstance instance

  final def provideSingleton[A <: AnyRef](provider: => A)(implicit c: ClassTag[A]) =
    provide[A](provider)(c) in SINGLETON

  final def provide[A <: AnyRef](provider: => A)(implicit c: ClassTag[A]) =
    bind(c.runtimeClass.asInstanceOf[Class[A]]) toProvider new Provider[A] { def get = provider }
}

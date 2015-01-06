package com.sos.scheduler.engine.kernel.variable

import com.google.common.collect.ImmutableMap

trait UnmodifiableVariableSet {
  def apply(name: String): String

  def size: Int

  def keys: Iterable[String]

  def toMap: Map[String, String]

  /** Für Java. */
  def toGuavaMap: ImmutableMap[String, String]

  /** Für Java. */
  def getNames: java.util.Collection[String]
}
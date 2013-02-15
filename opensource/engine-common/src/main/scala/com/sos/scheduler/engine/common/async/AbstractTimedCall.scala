package com.sos.scheduler.engine.common.async

abstract class AbstractTimedCall[A](val at: Long) extends TimedCall[A]

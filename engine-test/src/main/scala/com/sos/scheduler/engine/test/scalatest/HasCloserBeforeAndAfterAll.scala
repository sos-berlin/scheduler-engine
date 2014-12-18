package com.sos.scheduler.engine.test.scalatest

import com.sos.scheduler.engine.common.scalautil.HasCloser
import org.scalatest.{BeforeAndAfterAll, Suite}

trait HasCloserBeforeAndAfterAll extends HasCloser with BeforeAndAfterAll {
  this: Suite =>

  override def afterAll(): Unit = {
    closer.close()
    super.afterAll()
  }
}

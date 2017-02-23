package com.sos.scheduler.engine.test.scalatest

import com.sos.jobscheduler.common.scalautil.HasCloser
import org.scalatest.{BeforeAndAfterAll, Suite}

trait HasCloserBeforeAndAfterAll extends HasCloser with BeforeAndAfterAll {
  this: Suite ⇒

  override def afterAll() = {
    try closer.close()
    finally super.afterAll()
  }
}

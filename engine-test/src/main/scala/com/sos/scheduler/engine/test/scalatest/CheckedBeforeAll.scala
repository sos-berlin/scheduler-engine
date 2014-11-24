package com.sos.scheduler.engine.test.scalatest

import com.sos.scheduler.engine.test.scalatest.Utils.ignoreException
import org.scalatest.BeforeAndAfterAll

trait CheckedBeforeAll {
  this: BeforeAndAfterAll =>

  override protected final def beforeAll(): Unit = {
    try checkedBeforeAll()
    catch {
      case x: Throwable =>
        ignoreException { afterAll() }
        throw x
    }
  }

  /** Wie <code>BeforeAndAfterAll.beforeAll</code>, aber bei einer Exception wird <code>afterAll()</code> aufgerufen. */
  protected def checkedBeforeAll(): Unit = {}
}

package com.sos.scheduler.engine.test.scala

import org.scalatest.BeforeAndAfterAll
import com.sos.scheduler.engine.test.scala.Utils.ignoreException

trait CheckedBeforeAll {
  this: BeforeAndAfterAll =>

  override protected def beforeAll(configMap: Map[String, Any]) {
    try checkedBeforeAll(configMap)
    catch {
      case x =>
        ignoreException { afterAll(configMap) }
        throw x
    }
  }

  /** Wie <code>BeforeAndAfterAll.beforeAll</code>, aber bei einer Exception wird <code>afterAll()</code> aufgerufen. */
  protected def checkedBeforeAll() {}

  /** Wie <code>BeforeAndAfterAll.beforeAll</code>, aber bei einer Exception wird <code>afterAll()</code> aufgerufen. */
  protected def checkedBeforeAll(configMap: Map[String, Any]) {
    checkedBeforeAll()
  }
}

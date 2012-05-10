package com.sos.scheduler.engine.test.scala

import com.google.common.collect.ImmutableMap
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated
import com.sos.scheduler.engine.test.scala.Utils._
import com.sos.scheduler.engine.test.{ResourceToFileTransformer, TestSchedulerController, SchedulerTest}
import org.scalatest.{FunSuite, BeforeAndAfterAll}
import scala.collection.JavaConversions._

trait ScalaSchedulerTest extends FunSuite with BeforeAndAfterAll with EventHandlerAnnotated {
  val configurationPackage = getClass.getPackage
  val schedulerResourceToFileTransformer: ResourceToFileTransformer = null
  val schedulerResourceNameMap: Iterable[(String,String)] = List()
  lazy val controller = TestSchedulerController.builder(getClass)
      .resourcesPackage(configurationPackage)
      .nameMap(ImmutableMap.copyOf(mapAsJavaMap(schedulerResourceNameMap.toMap)))
      .resourceToFileTransformer(schedulerResourceToFileTransformer)
      .build

  def shortTimeout = SchedulerTest.shortTimeout   // Zur komfortableren Benutzung
  def injector = scheduler.getInjector

  override protected final def beforeAll(configMap: Map[String, Any]) {
    try {
      controller.getEventBus.registerAnnotated(this);
      checkedBeforeAll(configMap)
    }
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
    if (!controller.isStarted)
      controller.activateScheduler()
  }

  override def afterAll(configMap: Map[String, Any]) {
    try {
      controller.getEventBus.unregisterAnnotated(this);
      controller.close()
    }
    finally super.afterAll(configMap)
  }

  /** Zur Bequemlichkeit.
   * @see com.sos.scheduler.engine.test.TestSchedulerController#scheduler(). */
  protected final def scheduler = controller.scheduler
}

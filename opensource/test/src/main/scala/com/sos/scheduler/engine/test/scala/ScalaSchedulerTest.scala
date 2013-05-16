package com.sos.scheduler.engine.test.scala

import com.google.common.collect.ImmutableMap
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated
import com.sos.scheduler.engine.test.scala.Utils._
import com.sos.scheduler.engine.test.{ResourceToFileTransformer, TestSchedulerController, SchedulerTest}
import org.scalatest.{FunSuite, BeforeAndAfterAll}
import scala.collection.JavaConversions._
import scala.reflect.ClassTag
import com.sos.scheduler.engine.test.binary.CppBinariesDebugMode

trait ScalaSchedulerTest extends FunSuite with BeforeAndAfterAll with EventHandlerAnnotated {
  protected val configurationPackage = getClass.getPackage
  protected val schedulerResourceToFileTransformer: Option[ResourceToFileTransformer] = None
  protected val schedulerResourceNameMap: Iterable[(String,String)] = List()
  protected val binariesDebugMode: Option[CppBinariesDebugMode] = None
  protected val arguments: Seq[String] = Nil
  protected val logCategories: Option[String] = None
  lazy final val controller = {
    val b = TestSchedulerController.builder(getClass)
        .resourcesPackage(configurationPackage)
        .nameMap(ImmutableMap.copyOf(mapAsJavaMap(schedulerResourceNameMap.toMap)))
    schedulerResourceToFileTransformer foreach b.resourceToFileTransformer
    binariesDebugMode foreach b.debugMode
    logCategories foreach b.logCategories
    b.build
  }

  final def shortTimeout = SchedulerTest.shortTimeout   // Zur komfortableren Benutzung
  final def injector = scheduler.injector

  override protected final def beforeAll(configMap: Map[String, Any]) {
    try {
      controller.getEventBus.registerAnnotated(this)
      checkedBeforeAll(configMap)
    }
    catch {
      case x: Throwable =>
        ignoreException { afterAll(configMap) }
        throw x
    }
  }

  override protected final def beforeAll() {
    super.beforeAll()
  }

  /** Wie <code>BeforeAndAfterAll.beforeAll</code>, aber bei einer Exception wird <code>afterAll()</code> aufgerufen. */
  protected def checkedBeforeAll() {}

  /** Wie <code>BeforeAndAfterAll.beforeAll</code>, aber bei einer Exception wird <code>afterAll()</code> aufgerufen. */
  protected def checkedBeforeAll(configMap: Map[String, Any]) {
    checkedBeforeAll()
    if (!controller.isStarted)
      controller.activateScheduler(arguments: _*)
  }

  override def afterAll(configMap: Map[String, Any]) {
    try {
      controller.getEventBus.unregisterAnnotated(this)
      controller.close()
    }
    finally super.afterAll(configMap)
  }

  protected final def instance[A](implicit c: ClassTag[A]) =
    scheduler.injector.getInstance(c.runtimeClass.asInstanceOf[Class[A]])

  /** Zur Bequemlichkeit.
   * @see com.sos.scheduler.engine.test.TestSchedulerController#scheduler(). */
  protected final def scheduler = controller.scheduler
}

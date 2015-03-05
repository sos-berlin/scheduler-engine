package com.sos.scheduler.engine.taskserver.task

import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.cast
import com.sos.scheduler.engine.minicom.idispatch.{IDispatchFactory, IDispatchable, invocable}
import com.sos.scheduler.engine.minicom.types.{CLSID, IID, VariantArray}
import com.sos.scheduler.engine.taskserver.module.shell.{ShellModule, ShellModuleInstance}
import com.sos.scheduler.engine.taskserver.module.{ModuleInstance, NamedObjects, ShellModuleLanguage}
import java.util.UUID
import org.scalactic.Requirements._
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 * @see Com_remote_module_instance_server, spooler_module_remote_server.cxx
 */
final class RemoteModuleInstanceServer extends IDispatchable with HasCloser {
  import com.sos.scheduler.engine.taskserver.task.RemoteModuleInstanceServer._

  private var taskArguments: TaskArguments = null
  private var moduleInstance: ModuleInstance = null
  private var task: Task = null

  @invocable
  def construct(arguments: VariantArray): Unit = taskArguments = TaskArguments(arguments)

  @invocable
  def begin(objectAnys: VariantArray, objectNamesAnys: VariantArray): Unit = {
    moduleInstance = taskArguments.moduleLanguage match {
      case ShellModuleLanguage ⇒
        new ShellModuleInstance(new ShellModule(taskArguments.script), toNamedObjectMap(names = objectNamesAnys, anys = objectAnys))
    }
    task = newShellTask().closeWithCloser
    task.start()
  }

  private def newShellTask(): Task =
    moduleInstance match {
      case moduleInstance: ShellModuleInstance ⇒
        new ShellProcessTask(moduleInstance,
          jobName = taskArguments.jobName,
          hasOrder = taskArguments.hasOrder,
          environment = taskArguments.environment)
      case _ ⇒
        throw new IllegalArgumentException(s"Unknown language '${moduleInstance.module.moduleLanguage }'")
    }

  @invocable
  def end(succeeded: Boolean): Unit = {
    if (task != null)
      task.end()
  }

  @invocable
  def step(): String = task.step()

  @invocable
  def waitForSubprocesses(): Unit = {}

  override def toString =
    List(
      s"${getClass.getSimpleName}",
      Option(taskArguments) map { t ⇒ s"(${t.jobName}:${t.taskId}})" })
      .mkString("")
}

object RemoteModuleInstanceServer extends IDispatchFactory {
  val clsid = CLSID(UUID fromString "feee47a3-6c1b-11d8-8103-000476ee8afb")
  val iid   = IID  (UUID fromString "feee47a2-6c1b-11d8-8103-000476ee8afb")

  def apply() = new RemoteModuleInstanceServer

  private def toNamedObjectMap(names: VariantArray, anys: VariantArray): NamedObjects = {
    val nameStrings = names.as[String]
    val iDispatches = variantArrayToIDispatchables(anys)
    require(nameStrings.size == iDispatches.size)
    NamedObjects(nameStrings zip iDispatches)
  }

  /**
   * Expects an VariantArray with Some[IUnknown]
   * @return IUnknowns interpreted as IDispatchable
   * @throws NullPointerException when an IDispatch is null.
   */
  private def variantArrayToIDispatchables(a: VariantArray): immutable.IndexedSeq[IDispatchable] =
    a.indexedSeq.asInstanceOf[immutable.IndexedSeq[Some[_]]] map { case Some(o) ⇒ cast[IDispatchable](o) }
}

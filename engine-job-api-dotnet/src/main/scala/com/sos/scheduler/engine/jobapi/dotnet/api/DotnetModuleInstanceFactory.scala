package com.sos.scheduler.engine.jobapi.dotnet.api

/**
  * @author Joacim Zschimmer
  */
trait DotnetModuleInstanceFactory extends AutoCloseable {

  /**
    * @tparam A [[sos.spooler.Job_impl]] or a [[sos.spooler.Monitor_impl]]
    */
  @throws[Exception]
  def newInstance[A](clazz: Class[A], taskContext: TaskContext, reference: DotnetModuleReference): A
}

object DotnetModuleInstanceFactory {
  object Unsupported extends DotnetModuleInstanceFactory {
    def newInstance[A](clazz: Class[A], taskContext: TaskContext, reference: DotnetModuleReference) =
      throw new NotImplementedError(".Net is not supported")

    def close() = {}
  }
}

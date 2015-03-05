package com.sos.scheduler.engine.taskserver.module.java

import com.sos.scheduler.engine.taskserver.module.{JavaModuleLanguage, Module}

/**
 * @author Joacim Zschimmer
 */
final case class JavaModule(className: String) extends Module {
  override def moduleLanguage = JavaModuleLanguage
}

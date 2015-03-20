package com.sos.scheduler.engine.kernel.plugin

import com.sos.scheduler.engine.data.filebased.{AbsolutePath, FileBasedType}

/**
 * @author Joacim Zschimmer
 */
trait XmlConfigurationChangingPlugin extends Plugin {
  /**
   * The `FileBasedType`s, the plugin applies.
   */
  def fileBasedTypes: Set[FileBasedType]

  /**
   * When JobScheduler reads a XML configuration for a JobScheduler object, the plugin can change the configuration on the fly.
   */
  def changeXmlConfiguration(typ: FileBasedType, path: AbsolutePath, xmlBytes: Array[Byte]): Array[Byte]
}

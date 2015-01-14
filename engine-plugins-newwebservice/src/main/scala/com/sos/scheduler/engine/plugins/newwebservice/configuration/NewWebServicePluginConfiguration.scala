package com.sos.scheduler.engine.plugins.newwebservice.configuration

/**
 * @author Joacim Zschimmer
 */
final case class NewWebServicePluginConfiguration(
  testMode: Boolean = false)


object NewWebServicePluginConfiguration {
  private val Default = NewWebServicePluginConfiguration()

  final class Builder {
    var testMode = Default.testMode

    def build() = new NewWebServicePluginConfiguration(testMode = testMode)
  }
}

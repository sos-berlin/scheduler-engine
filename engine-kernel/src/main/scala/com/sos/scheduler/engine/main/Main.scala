package com.sos.scheduler.engine.main

import com.sos.scheduler.engine.kernel.CppScheduler
import com.sos.scheduler.engine.kernel.settings.CppSettings
import java.time.Duration

class Main {
  private final val schedulerController: SchedulerController = new SchedulerThreadController(classOf[Main].getName, CppSettings.Empty)

  private def apply(args: Seq[String]): Int = {
    CppScheduler.loadModuleFromPath()   // TODO Methode nur provisorisch. Besser den genauen Pfad übergeben, als Kommandozeilenparameter.
    schedulerController.startScheduler(args: _*)
    schedulerController.tryWaitForTermination(Duration.ofMillis(Long.MaxValue))
    schedulerController.exitCode
  }
}

object Main {
  def main(args: Array[String]): Unit = {
    val exitCode: Int = new Main().apply(args)
    if (exitCode != 0) throw new ExitCodeException(exitCode)
  }
}

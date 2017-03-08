package com.sos.scheduler.engine.tests.jira.js1695

/**
  * @author Joacim Zschimmer
  */
final class TestJob extends sos.spooler.Job_impl {
  override def spooler_process() = {
    if (java.lang.System.getProperty("os.name") startsWith "Windows") {
      locally {
        val subprocess = spooler_task.create_subprocess()
        subprocess.start("cmd.exe /c dir /b")
        subprocess.wait_for_termination()
        assert(subprocess.exit_code == 0)
      }
      //locally {
      //  Under Windows, user .start(String) !
      //  val subprocess = spooler_task.create_subprocess()
      //  val cmdExe = sys.env.get("ComSpec") orElse sys.env.get("COMSPEC" /*cygwin*/) getOrElse """C:\Windows\system32\cmd.exe"""
      //  subprocess.start(Array(cmdExe, "/c", "dir /b"))
      //  subprocess.wait_for_termination()
      //  assert(subprocess.exit_code == 0)
      //}
    } else {
      locally {
        val subprocess = spooler_task.create_subprocess()
        subprocess.start("ls -l")
        subprocess.wait_for_termination()
        assert(subprocess.exit_code == 0)
      }
      locally {
        val subprocess = spooler_task.create_subprocess()
        subprocess.start(Array("/bin/ls", "-l"))
        subprocess.wait_for_termination()
        assert(subprocess.exit_code == 0)
      }
    }
    false
  }
}

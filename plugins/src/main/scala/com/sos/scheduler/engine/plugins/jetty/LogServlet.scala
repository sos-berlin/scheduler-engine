package com.sos.scheduler.engine.plugins.jetty

import java.lang.Math.min
import javax.inject.Inject
import javax.servlet.http.{HttpServletResponse, HttpServletRequest, HttpServlet}
import com.google.inject.Singleton
import com.sos.scheduler.engine.kernel.folder.AbsolutePath
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.schedulerEncoding
import com.sos.scheduler.engine.kernel.scheduler.SchedulerInstanceId
import java.io.{File, FileInputStream, InputStream, OutputStream}
import java.nio.charset.Charset

@Singleton
//@WebServlet(Array("command"))
class LogServlet @Inject()(jobSubsystem: JobSubsystem, schedulerInstanceId: SchedulerInstanceId) extends HttpServlet {
  import LogServlet._

  override def doGet(request: HttpServletRequest, response: HttpServletResponse) {
    val jobPath = AbsolutePath.of(stripFromEnd(request.getPathInfo, ".job/log"))
    val file = jobSubsystem.job(jobPath).getLog.getFile
    respondWithFile(response, file, schedulerEncoding)
  }

  private def respondWithFile(response: HttpServletResponse, file: File, encoding: Charset) {
    val in = new FileInputStream(file)
    try {
      val byteCount = file.length
      response.setStatus(HttpServletResponse.SC_OK)
      response.addHeader("Content-Length", byteCount.toString);
      response.setContentType("text/xml;charset="+encoding)
      response.setHeader("ETag", schedulerInstanceId.getString +"."+byteCount);
      copyLimited(in, response.getOutputStream, byteCount)
    }
    finally in.close()
  }
}

object LogServlet {
  private def stripFromEnd(s: String, end: String) = s ensuring {_ endsWith end} substring (0, s.length - end.length)

  private def copyLimited(input: InputStream, output: OutputStream, limit: Long): Long = {
    val buffer = new Array[Byte](4096)
    var written: Long = 0
    while (written < limit) {
      val n = input.read(buffer, 0, min(limit - written, buffer.size).toInt)
      if (n == -1)  return written
      output.write(buffer, 0, n)
      written += n
    }
    written
  }
}

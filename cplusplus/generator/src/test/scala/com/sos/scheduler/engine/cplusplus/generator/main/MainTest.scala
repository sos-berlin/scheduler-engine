package com.sos.scheduler.engine.cplusplus.generator.main

import com.sos.scheduler.engine.cplusplus.generator.Configuration._
import com.sos.scheduler.engine.cplusplus.scalautil.io.FileUtil._
import java.io.File
import org.junit.Test
import org.junit.Assert._
import scala.collection.JavaConversions._
import Main._


class MainTest {
    private val tmp = new File(System.getProperty("java.io.tmpdir"))        // Warum ist das unter Maven das Home-Verzeichnis?
    System.err.println("java.io.tmpdir=" + tmp)
    requireDirectoryExists(tmp,"Temporary directory")
    //if (List("tmp", "temp") forall { a => !(tmp.toString.toLowerCase contains a)}) throw new RuntimeException("Seems to be an invalid temporary directory: " + tmp)
    
    private val cppOutputDirectory = new File(tmp, "proxy.c++")
    private val javaOutputDirectory = new File(tmp, "proxy.java")
    private val cppProxyClassNames = List(
        "com.sos.scheduler.engine.kernel.cppproxy.SpoolerC",
        "com.sos.scheduler.engine.kernel.Scheduler")

    cppOutputDirectory.mkdir()
    new File(cppOutputDirectory, cppSubdirectory).mkdir()
    javaOutputDirectory.mkdir()
    cppProxyClassNames foreach { a => new File(javaOutputDirectory, a.replace('.', '/')).getParentFile.mkdirs() }
    new File(cppOutputDirectory,"scheduler").mkdir()

//    @Test def testA() {
//        val urls = classOf[MainTest].getClassLoader.getResources("com/sos/scheduler/kernel/core/cppproxy").toList
////        def classNameOfUrl(u: URL) = u.openConnection().asInstanceOf[JarURLConnection].getEntryName.replace('/','.')
//        def entryNameOfUrl(u: URL) = u.toString.split('!').last.replace('/','.')
//    }

    @Test def testIsClassName() {
        assert(isClassName("A"))
        assert(isClassName("a.B"))
        assert(isClassName("a.Bb"))
        assert(!isClassName("a"))
        assert(!isClassName("a."))
        assert(!isClassName("a.bb"))
        assert(!isClassName(""))
        assert(!isClassName("."))
        assert(!isClassName("a."))
    }

    @Test def testMain {
        new Main(Array(
            //"-deep",
            "-c++-output-directory=" + cppOutputDirectory,
            "-java-output-directory=" + javaOutputDirectory,
            "java.math.BigInteger",
            "java.io.File",
            "java.io.FileWriter",
            "java.io.FileReader")
            ++ cppProxyClassNames
        ).apply()
    }
}

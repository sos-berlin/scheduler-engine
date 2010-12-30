package com.sos.scheduler.kernel.cplusplus.generator.cppproxy

import com.sos.scheduler.kernel.cplusplus.generator.Configuration._
import com.sos.scheduler.kernel.cplusplus.generator.module._
import com.sos.scheduler.kernel.cplusplus.generator.visualstudio.VisualStudio
import com.sos.scheduler.kernel.cplusplus.scalautil.io.FileUtil._
import java.io.File
import Generator._


/** Generator für Java- und C++-Code der C++-Proxys, also der in Java zu nutzenden C++-Klassen. */
class Generator(cppOutputDirectory: Option[File], javaOutputDirectory: Option[File], interfaces: Iterable[Class[_]])
{
    javaOutputDirectory foreach { d => requireDirectoryExists(d, "Java") }
    cppOutputDirectory foreach { d => requireDirectoryExists(d, "C++") }

    private val (javaModules, jniModules) = {
        val modulePairs = interfaces.toList sortBy { _.getName } map { new CppProxyModulePair(_) }
        modulePairs unzip { a => (a.javaModule, a.jniModule)}
    }

    def apply() { 
        javaOutputDirectory foreach generateJava
        cppOutputDirectory foreach generateJni
    }

    private def generateJava(dir: File) {
        javaModules foreach { _.writeToDirectory(dir) }
        JavaModule.removeFilesBut(dir, javaModules)
    }

    private def generateJni(dir: File) {
        for ((subdirectory, subJniModules) <- jniModules groupBy { _.subdirectory })
            generateJniForSubdirectory(new File(dir, subdirectory), subJniModules)
    }
}


object Generator {
    private def generateJniForSubdirectory(subdir: File, subJniModules: List[JniModule]) {
        subJniModules foreach { _.writeToDirectory(subdir) }

        val registerNativeClassesModule = {
            val includes = (subJniModules flatMap { _.include }).distinct    // Wir mischen einfach alle zusammen (z.B. spooler.h)
            new RegisterNativeClassesModule(includes, subJniModules)
        }
        registerNativeClassesModule.writeToDirectory(subdir)

        val modules = registerNativeClassesModule :: subJniModules

        new MakefileInclude(prefix="jni_cppproxy", modules).writeToDirectory(subdir)
        VisualStudio.updateProjectFiles(subdir, modules)
        JniModule.removeFilesBut(subdir, modules)
    }
}

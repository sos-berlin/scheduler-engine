package com.sos.scheduler.kernel.cplusplus.generator.javaproxy

import com.sos.scheduler.kernel.cplusplus.generator.Configuration._
import com.sos.scheduler.kernel.cplusplus.generator.javaproxy.clas._
import com.sos.scheduler.kernel.cplusplus.generator.module._
import com.sos.scheduler.kernel.cplusplus.generator.util.ClassOps._
import com.sos.scheduler.kernel.cplusplus.generator.util.Util._
import com.sos.scheduler.kernel.cplusplus.generator.visualstudio.VisualStudio
import com.sos.scheduler.kernel.cplusplus.scalautil.io.FileUtil._
import java.io.File


/** Generator für C++-Code der Java-Proxys, also der in C++ zu nutzenden Java-Klassen. */
class Generator(outputDirectory: File, classes: Set[Class[_]], deep: Boolean=false) {
    requireDirectoryExists(outputDirectory, "C++")
    
    private val javaProxyOutputDirectory = new File(outputDirectory, cppSubdirectory)

    private val knownClasses: Set[Class[_]] = {
        val allClasses = classes ++ extraNeededClasses
        val otherClasses = if (deep) allDependendClasses _  else neededClasses _
        allClasses ++ (allClasses flatMap otherClasses)
    }

    private val pch = new PrecompiledHeaderModule(javaProxyOutputDirectory)

    private val cppModules = {
        val cppClasses = knownClasses.toList sortBy { _.getName } map { new CppClass(_, knownClasses) }
        cppClasses map { c => new JavaProxyCppModule(c, knownClasses, pch.includeHeader) }
    }
    
    def apply() {
        cppModules foreach { _.writeToDirectory(javaProxyOutputDirectory) }
        pch.writeToDirectory(javaProxyOutputDirectory)
        val makefileInclude = new MakefileInclude(prefix="javaproxy", cppModules)
        makefileInclude.writeToDirectory(javaProxyOutputDirectory)
        VisualStudio.updateProjectFiles(javaProxyOutputDirectory, cppModules)
        
        CppModule.removeFilesBut(javaProxyOutputDirectory, makefileInclude :: pch :: cppModules)
    }
}

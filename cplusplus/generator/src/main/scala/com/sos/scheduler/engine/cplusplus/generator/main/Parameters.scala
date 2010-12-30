package com.sos.scheduler.engine.cplusplus.generator.main

import java.io.File
import scala.collection.mutable

/** Parameter zur Ausführung des Generators */
class Parameters(
    val cppOutputDirectory: Option[File],
    val javaOutputDirectory: Option[File],
    val classOrPackageNames: Set[String],
    val deep: Boolean
)


object Parameters
{
    private val optionWithValue = """-([^=]+)=(.*)""".r
    private val option = """-(.*)""".r

    def ofCommandLine(args: Array[String]): Parameters = {
        var cppOutputDirectory: Option[File] = None
        var javaOutputDirectory: Option[File] = None
        val classOrPackageNames = mutable.HashSet[String]()
        var deep = false

        for (arg <- args) arg match {
            case optionWithValue(name, value) => name match {
                case "c++-output-directory" => cppOutputDirectory = optionalFile(value)
                case "java-output-directory" => javaOutputDirectory = optionalFile(value)
                case unknownName => throw new RuntimeException("Unbekannte Option -" + unknownName + "=")
            }
            case option(name) => name match {
                case "?" => System.err.println(usage)
                case "deep" => deep = true
                case unknownName => throw new RuntimeException("Unbekannte Option -" + unknownName)
            }
            case value => classOrPackageNames += value
        }

        new Parameters(
            cppOutputDirectory = cppOutputDirectory,
            javaOutputDirectory = javaOutputDirectory,
            classOrPackageNames = classOrPackageNames.toSet,
            deep = deep)
    }


    def usage = "java ... " + classOf[Main].getName + "\n"
        "    -cpp-output-directory=\n" +
        "    -java-output-directory=\n" +
        "    CLASSES...\n"


    private def optionalFile(x: String) = if (x.isEmpty) None  else Some(new File(x))
}

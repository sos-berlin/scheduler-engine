package com.sos.scheduler.kernel.cplusplus.scalautil.io

import java.io._
import java.nio.charset.Charset
import org.apache.commons.io.FileUtils._
import Util._


object FileUtil {
    def readingFile[A](file: File, encoding: Charset)(writeFunction: Reader => A): A = {
        val input = new InputStreamReader(new FileInputStream(file), encoding)
        closingFinally(input)(writeFunction)
    }

    def writingFile[A](file: File, encoding: Charset)(writeFunction: Writer => A): A = {
        val output = new OutputStreamWriter(new FileOutputStream(file), encoding)
        closingFinally(output)(writeFunction)
    }

    def writingFileIfDifferent(file: File, encoding: Charset)(writeFunction: Writer => Unit): Boolean = {
        val tempFile = new File(file + "~")
        var ok = false
        var written = false

        try {
            writingFile(tempFile, encoding)(writeFunction)

            if (contentEquals(tempFile, file))
                tempFile.delete
            else {
                moveOrCopyFile(tempFile, file)  //Manchmal funktioniert tempFile.moveTo(file) nicht, deshalb copyFile.
                written = true
            }

            ok = true
        }
        finally if (!ok) tempFile.delete

        written
    }

    def withTemporaryFile[A](prefix: String, suffix: String = ".tmp")(f: File => A): A = {
        val file = File.createTempFile(prefix, suffix)
        try f(file)
        finally file.delete
    }

//    def withTemporaryFile[A](file: File)(f: File => A): A = try f(file) finally file.delete

    def moveOrCopyFile(from: File, to: File) = from.renameTo(to)  || {
        copyFile(from, to);
        from.delete
    }

    def deleteFile(file: File) {
        val ok = file.delete
        if (!ok) {
            if (!file.exists)   throw new RuntimeException("File to be deleted does not exist: " + file)
            if (file.isDirectory)   throw new RuntimeException("File to be deleted is a (non empty?) directory: " + file)
            throw new RuntimeException("File cannot be deleted: " + file)
        }
    }

    def requireFileExists(f: File, argumentName: String) {
        if (f.toString.isEmpty)  throw new IllegalArgumentException("Missing argument '" + argumentName + '"')
        if (!f.exists)  throw new IllegalArgumentException("Non existant file or directory for '" + argumentName + "'", new FileNotFoundException(f.toString))
    }

    def requireDirectoryExists(f: File, argumentName: String = "") {
        if (f.toString.isEmpty)  throw new IllegalArgumentException("Missing argument '" + argumentName + '"')
        if (!f.exists)  throw new IllegalArgumentException("Non existant directory for '" + argumentName + "'", new FileNotFoundException(f.toString))
        if (!f.isDirectory)  throw new IllegalArgumentException("Is not an directory for '" + argumentName + "': " + f)
    }

    def listFilePathsRecursive(dir: File) = listFilePathsRecursiveFiltered(dir){ _ => true }

    /** Liefert nicht die Verzeichnisnamen.
     * (Vielleicht sollten Verzeichnisnamen mit "/" am Ende geliefert werden?)
     */
    def listFilePathsRecursiveFiltered(dir: File)(fileIsRelevant: File => Boolean): Seq[String] = {
        dir.listFiles filter fileIsRelevant flatMap { f =>
            val name = f.getName
            if (f.isDirectory)  listFilePathsRecursiveFiltered(f)(fileIsRelevant) map { name + "/" + _ }
            else Seq(name)
        }
    }
}

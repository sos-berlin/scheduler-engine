package com.sos.scheduler.engine.cplusplus.generator.visualstudio

import org.junit._
import com.sos.scheduler.engine.cplusplus.generator.Configuration._
import com.sos.scheduler.engine.cplusplus.generator.module.CppModule
import com.sos.scheduler.engine.cplusplus.scalautil.io.FileUtil._
import java.io.File
import org.apache.log4j.Logger
import org.hamcrest.Matchers._
import org.junit.Assert._
import scala.xml.Elem
import scala.xml.Utility.trimProper
import scala.xml.XML


class VcprojTest {
    val vs2008document = TestDoc(
        <VisualStudioProject ProjectType="Visual C++">
            <Platforms>
                <Platform Name="Win32"/>
            </Platforms>
            <Files>
                <Filter Name="Quellcodedateien">
                    <File RelativePath="_precompiled_headers.cxx"/>
                    <Filter Name={visualStudioFilterName}>
                        <File RelativePath=".\same.cxx"/>
                        <File RelativePath=".\old.cxx"/>
                    </Filter>
                </Filter>
            </Files>
            <Globals/>
        </VisualStudioProject>,
        
        <VisualStudioProject ProjectType="Visual C++">
            <Platforms>
                <Platform Name="Win32"/>
            </Platforms>
            <Files>
                <Filter Name="Quellcodedateien">
                    <File RelativePath="_precompiled_headers.cxx"/>
                    <Filter Name={visualStudioFilterName}>
                        <File RelativePath="new.cxx"/>
                        <File RelativePath="same.cxx"/>
                    </Filter>
                </Filter>
            </Files>
            <Globals/>
        </VisualStudioProject>)

    val vs2010document = TestDoc(
        <Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
            <ItemGroup>
                <ClCompile Include="_precompiled.cxx"/>
                <ClCompile Include="old.cxx"/>
                <ClCompile Include="same.cxx"/>
            </ItemGroup>
        </Project>,

        <Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
            <ItemGroup>
                <ClCompile Include="_precompiled.cxx"></ClCompile>
                <ClCompile Include="same.cxx"></ClCompile>
                <ClCompile Include="new.cxx"></ClCompile>
            </ItemGroup>
        </Project>)

    val vs2010FilterDocument = TestDoc(
        <Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
            <ItemGroup>
                <ClCompile Include="_precompiled.cxx">
                    <Filter>Quelldateien</Filter>
                </ClCompile>
                <ClCompile Include="old.cxx">
                    <Filter>Quelldateien\{visualStudioFilterName}</Filter>
                </ClCompile>
                <ClCompile Include="same.cxx">
                    <Filter>Quelldateien\{visualStudioFilterName}</Filter>
                </ClCompile>
            </ItemGroup>
        </Project>,

        <Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
            <ItemGroup>
                <ClCompile Include="_precompiled.cxx">
                    <Filter>Quelldateien</Filter>
                </ClCompile>
                <ClCompile Include="same.cxx">
                    <Filter>Quelldateien\{visualStudioFilterName}</Filter>
                </ClCompile>
                <ClCompile Include="new.cxx">
                    <Filter>Quelldateien\{visualStudioFilterName}</Filter>
                </ClCompile>
            </ItemGroup>
        </Project>)

    @Test def test2008 {
        testFunction(List(vs2008document), updated2008Doc, "Visual Studio 2008")
    }

    @Test def test2010 {
        testFunction(List(vs2010document, vs2010FilterDocument), updated2010Doc, "Visual Studio 2010")
    }

    def testFunction(testDocs: List[TestDoc], f: (List[TestDoc], List[String]) => List[Result], what: String) {
        val results = f(testDocs, List("new.cxx", "same.cxx"))
        for ((testDoc, r) <- testDocs zip results) {
            val failReason = what + " " + r.file.getName + " is not correctly updated"
            //log.info(what + " " + r.file.getName + ":\n" + r.xml)
            assertThat(failReason, r.xml, not(containsString("old.cxx")))
            assertThat(failReason, r.xml, ContainsSingleString("new.cxx"))
            assertThat(failReason, r.xml, ContainsSingleString("same.cxx"))
            assertEquals(compactXml(testDoc.expected), r.xml)   // Könnte versagen, wenn Reihenfolge geändert wird
        }
    }

    private def updated2008Doc(testDocs: List[TestDoc], paths: List[String]) = withTemporaryFile("test-", ".vcproj") { file =>
        xml.XML.save(file.getPath, testDocs(0).source, "UTF-8")
        val vs = new VcprojFile(file, paths)
        List(Result(compactXml(vs.updatedDocument), file))
    }

    private def updated2010Doc(testDocs: List[TestDoc], paths: List[String]) = withTemporaryFile("test", ".vcxproj") { file =>
        val filterFile = new File(file.getPath + ".filters")
        try {
            xml.XML.save(file.getPath, testDocs(0).source, "UTF-8")
            xml.XML.save(filterFile.getPath, testDocs(1).source, "UTF-8")
            val vs = new VcxprojFile(file, paths)
            vs.updatedDocs map { doc => Result(compactXml(XML.loadString(doc.toXml)), doc.file) }
        }
        finally filterFile.delete
    }

    private def compactXml(a: Elem) = trimProper(a).toString

    
    case class TestDoc(source: Elem, expected: Elem)

    class MyModule(val name: String) extends CppModule {
        protected val headerCodeOption = None
        protected val sourceCodeOption = Some("test")
    }

    case class Result(xml: String, file: File)
}


object VcprojTest {
    private val log = Logger.getLogger(getClass)
}
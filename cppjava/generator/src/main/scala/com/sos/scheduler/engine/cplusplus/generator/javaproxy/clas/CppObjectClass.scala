package com.sos.scheduler.engine.cplusplus.generator.javaproxy.clas

import com.sos.scheduler.engine.cplusplus.generator.cpp._
import com.sos.scheduler.engine.cplusplus.generator.cpp.javabridge.JavaBridge
import com.sos.scheduler.engine.cplusplus.generator.javaproxy.procedure.CppConstructor
import com.sos.scheduler.engine.cplusplus.generator.util.ClassOps
import com.sos.scheduler.engine.cplusplus.generator.util.MyRichString._
import com.sos.scheduler.engine.cplusplus.generator.util.ProcedureSignature

/** Liefert die Klasse, die das Objekt beschreibt. Das ist also die eigentliche Proxy-Klasse */
class CppObjectClass(cppClass: CppClass) extends CppCode {
    val name = cppClass.javaClass.getSimpleName
    private val classClassName = cppClass.cppClassClass.name

    private val javaSuperclass: Option[Class[_]] = {
        def knownSuperclass(clas: Class[_]): Option[Class[_]] = clas.getSuperclass match {
            case null => ClassOps.superclass(clas)
            case superclass => if (cppClass.knownClasses contains superclass) Some(superclass)  else knownSuperclass(superclass)
        }

        knownSuperclass(cppClass.javaClass)
    }

    private val hasSuperclass = javaSuperclass.isDefined

    private val superclassName = javaSuperclass match {
        case Some(superclass) => CppName(superclass).fullName
        case None             => JavaBridge.namespace + "::Class_based"
    }
    
    private val jobjectSuperclassName = JavaBridge.hasProxyJobjectClassName(name)

    def forwardDeclaration = ""

    def extraUsedJavaClasses = objNameJavaClasses

    private val codeSnippets: List[CppCode] = List(
        staticObjectInstantiators,
        constructorsCode,
        copyConstructorCode,
        moveConstructorCode,
        destructorCode,
        assignmentOperatorsCode,
        jobjectGetterCode,
        jobjectSetterCode,
        methodsCode,
        javaObjectClassMethodCode,
        javaClassMethodCode,
        objNameMethodCode,
        variablesCode)
    
    def headerCode =
        "struct " + name + " : " + jobjectSuperclassName + ", " + superclassName + " {\n" +
        (codeSnippets map { _.headerCode }).mkString("\n") +
        "};\n\n"

    def sourceCode = (codeSnippets map { _.sourceCode + "\n" }).mkString

    private def staticObjectInstantiators = new CppCode {
        def headerCode = if (cppClass.cppConstructors.isEmpty) hidingHeaderCode  else headerCode2
        def headerCode2 = (cppClass.cppConstructors map { "    " + _.objectInstantiator.headerCode + "\n" }).mkString
        def sourceCode  = (cppClass.cppConstructors map {          _.objectInstantiator.sourceCode + "\n" }).mkString
        def defaultConstructorSignature = ProcedureSignature.ofConstructor(Nil)

        /** Versteckt new_instance() der Oberklasse, wenn sonst kein new_instance definiert wird. */
        private def hidingHeaderCode =
            "  private:\n" +
            "    " + new CppConstructor(cppClass, defaultConstructorSignature).objectInstantiator.headerCode + "  // Not implemented\n" +
            "  public:\n"
    }

    protected def constructorsCode = new CppCode {
        def headerCode = "    " + name + "(jobject = NULL);\n"
        def sourceCode = name + "::" + name + "(jobject jo) { if (jo) assign_(jo); }\n"
    }

    protected def copyConstructorCode = new CppCode {
        def headerCode = "    " + name + "(const " + name + "&);\n"
        def sourceCode = name + "::" + name + "(const " + name + "& o) { assign_(o.get_jobject()); }\n"
    }

    protected def moveConstructorCode = new CppCode {
        def headerCode = 
            "    #ifdef Z_HAS_MOVE_CONSTRUCTOR\n" +
            "        " + name + "(" + name + "&&);\n" +
            "    #endif\n"
        def sourceCode = 
            "#ifdef Z_HAS_MOVE_CONSTRUCTOR\n" +
            "    " ++ name + "::" + name + "(" + name + "&& o) { set_jobject(o.get_jobject());  o.set_jobject(NULL); }\n" +
            "#endif\n"
    }

    protected def destructorCode = new CppCode {
        def headerCode = "    ~" + name + "();\n"
        def sourceCode = name + "::~" + name + "() { assign_(NULL); }\n"
    }

    private def assignmentOperatorsCode = new CppCode {
        def headerCode =
            "    " + name + "& operator=(jobject jo) { assign_(jo); return *this; }\n" +
            "    " + name + "& operator=(const " + name + "& o) { assign_(o.get_jobject()); return *this; }\n" +
            "    #ifdef Z_HAS_MOVE_CONSTRUCTOR\n" +
            "        " + name + "& operator=(" + name + "&& o) { set_jobject(o.get_jobject()); o.set_jobject(NULL); return *this; }\n" +
            "    #endif\n"
        def sourceCode = ""
    }

    private def assignCode = new CppCode {
        def headerCode = "    void assign_(jobject jo) { " + superclassName + "::assign_(jo); " + jobjectSuperclassName + "::assign_(jo); }\n"
        def sourceCode = ""
    }

    private def jobjectGetterCode = new CppCode {
        def headerCode = "    jobject get_jobject() const { return " + jobjectSuperclassName + "::get_jobject(); }\n"
        def sourceCode = ""
    }

    private def jobjectSetterCode = new CppCode {
        def headerCode =
            "  protected:\n" +
            "    void set_jobject(jobject jo) {\n" +
            "        " + jobjectSuperclassName + "::set_jobject(jo);\n" +
           ("        " + superclassName + "::set_jobject(jo);\n" when hasSuperclass) +
            "    }\n" +
            "  public:\n"
        def sourceCode = ""
    }

    private def methodsCode = new CppCode {
        def headerCode = (cppClass.cppMethods map { "    " + _.objectClassCode.headerCode + "\n" }).mkString
        def sourceCode = (cppClass.cppMethods map { _.objectClassCode.sourceCode + "\n" }).mkString
    }

    private def javaObjectClassMethodCode = new CppCode {
        def headerCode = "    " + JavaBridge.namespace + "::Class* java_object_class_();\n"
        def sourceCode = JavaBridge.namespace + "::Class* " + name + "::java_object_class_() { return _class.get(); }\n"
    }

    private def javaClassMethodCode = new CppCode {
        def headerCode = "    static " + JavaBridge.namespace + "::Class* java_class_();\n"
        def sourceCode = JavaBridge.namespace + "::Class* " + name + "::java_class_() { " +
            "return " + classClassName + "::class_factory.clas(); }\n"
    }

    private def objNameMethodCode = CppCode.empty       // Was ist mit obj_name()? Muss es const sein?
//    new CppCode {
//        def headerCode = "    ::std::string obj_name() const;  // ruft toString()\n"
//        def sourceCode = "::std::string " + name + "::obj_name() const { return toString(); }\n"
//    }

    private val objNameJavaClasses = Set(classOf[String])

    private def variablesCode = new CppCode {
        def headerCode =
            "  private:\n" +
            "    struct Lazy_class : ::zschimmer::abstract_lazy<" + classClassName + "*> {\n" +
            "        void initialize();\n" +
            "    };\n" +
            "\n" +
            "    Lazy_class _class;\n";
        def sourceCode =
            "void " + name + "::Lazy_class::initialize() {\n" +
            "    _value = " + classClassName + "::class_factory.clas();\n" +
            "}\n"
    }
}

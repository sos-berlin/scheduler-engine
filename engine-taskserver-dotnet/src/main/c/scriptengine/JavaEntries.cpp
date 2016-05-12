#include <string>
#include <comdef.h>
#include <comutil.h>
#include <comdefsp.h>
#include <jni.h>

using namespace std;


jstring JNICALL Java_com_sos_engine_taskserver_windows_scriptengine_iDispatchToJava(JNIEnv* jniEnv, jclass, jlong iDispatch);

namespace java {
    JavaVM* staticVm = NULL;

    _bstr_t javaReturnException(int error, const char* functionName) {
        _bstr_t text;
        switch (error) {
            case JNI_OK:        text = "ret=JNI_OK";                                            break;
            case JNI_ERR:       text = "ret=JNI_ERR";                                           break;
            case JNI_EDETACHED: text = "ret=JNI_EDETACHED \"thread detached from the VM\"";     break;
            case JNI_EVERSION:  text = "ret=JNI_EVERSION \"JNI version error\"";                break;
            case -4:            text = "ret=JNI_ENOMEM \"not enough memory\"";                  break;
            case -5:            text = "ret=JNI_EEXIST \"VM already created\"";                 break;
            case -6:            text = "ret=JNI_EINVAL \"invalid arguments\"";                  break;
            default:            text = "ret=" + intToBstr(error);
        }
        return _bstr_t("Error " + text + ", in Java call " + functionName);
    }

    JNIEnv* jni() {
        if (!staticVm) {
            string msg = "FATAL ERROR: Java Native Interface (JNI) is not available";
            fprintf(stderr, msg.c_str());
            throw _bstr_t(msg.c_str());
        }
        JNIEnv* result;
        staticVm->GetEnv((void**)&result, JNI_VERSION_1_8);
        if (!result) {
            string msg = "FATAL ERROR: Java Native Interface (JNI) is detached";
            fprintf(stderr, msg.c_str());
            throw _bstr_t(msg.c_str());
        }
        return result;
    }

    struct LocalFrame {
        LocalFrame(int objectCount) {
            int error = jni()->PushLocalFrame(objectCount); 
            if (error)  throw javaReturnException(error, "PushLocalFrame"); 
        }
        
        ~LocalFrame() { 
            jni()->PopLocalFrame(NULL); 
        }
    };


    _bstr_t lastJavaException() {
        JNIEnv* j = jni();
        if (jthrowable throwableJ = jni()->ExceptionOccurred()) {
            #if !defined Z_AIX  // VMJNCK028E JNI error in PushLocalFrame: This function cannot be called when an exception is pending
                LocalFrame localFrame(10);
            #endif

            //if (jthrowable x = j->ExceptionOccurred()) {
            //    j->ExceptionClear();
            //    if (jclass c = j->GetObjectClass(x)) {
            //        LocalRef cls = j->CallObjectMethod(x, j->GetMethodID(c, "getClass", "()Ljava/lang/Class;"));
            //        if (!j->ExceptionCheck()) {
            //            LocalRef name_j((jstring)j->CallObjectMethod(cls, j->GetMethodID(cls.get_jclass(), "getName", "()Ljava/lang/String;")));
            //            if (!j->ExceptionCheck()) {
            //                xc._exception_name = string_from_jstring(name_j);
            //                while (1) {
            //                    jthrowable x2 = NULL;
            //                    jmethodID toString_id = j->GetMethodID(c, "toString", "()Ljava/lang/String;");
            //                    if (!toString_id)  break;

            //                    Local_jstring string_j((jstring)j->CallObjectMethod(x, toString_id));
            //                    if (!string_j || j->ExceptionCheck())  break;

            //                    xc._message += string_from_jstring(string_j);
            //                    jmethodID get_cause_id = j->GetMethodID(c, "getCause", "()Ljava/lang/Throwable;");
            //                    if (!get_cause_id)  break;

            //                    x2 = (jthrowable)j->CallObjectMethod(x, get_cause_id);
            //                    if (j->ExceptionCheck())  break;
            //                    if (!x2)  break;

            //                    xc._message += " - caused by - ";

            //                    j->DeleteLocalRef(x);
            //                    x = x2;
            //                }
            //            }
            //        }

            //        j->DeleteLocalRef(c);
            //    }

            //    if( x )  jenv->DeleteLocalRef( x );
            //} else {
            //}
        }
        j->ExceptionClear();
        return "Error in Java call";
    }

    void checkForJavaError() {
        if (jni()->ExceptionOccurred()) {  // ExceptionCheck?
            throw lastJavaException();
        }
    }

    //extern "C" jint JNI_OnLoad(JavaVM *vm, void*) {
    //    jni = vm->GetEnv((void**)&jni, JNI_VERSION_1_8);
    //}

    //extern "C" void JNI_OnUnload(JavaVM *vm, void*) {
    //    jni = NULL;
    //}

    struct LocalRef {
        LocalRef(jobject o) : 
            _ref(o) 
        {}
        
        ~LocalRef() {
            if (_ref) jni()->DeleteLocalRef(_ref);
        }

        private:
        jobject const _ref;
    };

    struct GlobalRef {
        GlobalRef(jobject o) : 
            _ref(o) 
        {}
        
        ~GlobalRef() {
            if (_ref) jni()->DeleteGlobalRef(_ref);
        }

        jobject ref() {
            return _ref;
        }

        private:
        jobject const _ref;
    };

    jobject toGlobalRef(jobject o) {
        jobject global = jni()->NewGlobalRef(o);
        if (!global) throw lastJavaException();
        return global;
    }

    jclass findClass(const string& name) {
        jclass c = jni()->FindClass(name.c_str());
        if (!c) throw lastJavaException();
        return c;
    }

    struct JavaClass {
        JavaClass(const string& name) : 
            name(name),
            classJ((jclass)toGlobalRef(findClass(name)))
        {
            if (!classJ) throw lastJavaException();
        }

        ~JavaClass() {
            jni()->DeleteGlobalRef(classJ);
        }

        private:
        jclass classJ;
        string name;
    };

    struct JavaObject {
        private:
        JavaClass javaClass;
    };
}

using namespace java;

struct IDispatchAdapterClassJ {
    IDispatchAdapterClassJ(jclass classJ) :
        classJ(classJ),
        getIdOfName_id(jni()->GetMethodID(getJclass(), "invoke", "(Ljava/lang/String;)")),
        invoke_id(jni()->GetMethodID(getJclass(), "invoke", "(Ljava/lang/String;)"))
    {}

    jclass getJclass() const {
        return (jclass)classJ.ref();
    }

    GlobalRef const classJ;
    jmethodID const invoke_id;
    jmethodID const getIdOfName_id;
};

struct IDispatchAdapterJ {
    
    DISPID getIdOfName(const _bstr_t& name) {
        return jni()->CallIntMethod(classJ.getJclass(), classJ.getIdOfName_id, bstrToJstring(name));
    }

    DISPID invoke(const _bstr_t& name) {
        return jni()->CallIntMethod(classJ.getJclass(), classJ.getIdOfName_id, bstrToJstring(name));
    }


    private:
    IDispatchAdapterClassJ classJ;
};


jobject iDispatchToJava(IDispatch* iDispatch) {
     return NULL; // ?????????????????????????
}

void releaseIDispatch(jobject iDispatchJ) {
    // ??????????????????????????
}

_bstr_t jstringToBstr(jstring s) {
    return "JAVA-STRING";
}

struct __declspec(uuid("2d324d31-938e-4c79-90fd-619897a7ca28")) JavaIDispatchAdapter : IDispatch {
    JavaIDispatchAdapter(jobject iDispatchJ) : 
        iDispatchJ(iDispatchJ),
        referenceCount(0)
    {
        AddRef();
    }

    ULONG STDMETHODCALLTYPE AddRef() {
        if (iDispatchJ) {
            referenceCount++;
        }
    }
    ULONG STDMETHODCALLTYPE Release() {
        if (iDispatchJ) {
            referenceCount--;
            if (referenceCount == 0) {
                javaRelease(iDispatchJ);
                delete this;
            }
        }
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) {
        if (riid == IID_IDispatch) {
            AddRef();
            *ppvObject = (IDispatch*)this;
            return S_OK;
        } else {
            *ppvObject = NULL;
            return E_NOINTERFACE;
        }
    }


    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo) {
        *pctinfo = 0;
        return S_OK;
    }
        
    HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo) {
        *ppTInfo = NULL;
        return E_FAIL;
    };
        
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId) {
    }
    
    HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr) {
    }

    jobject asJobject() {
        AddRef();
        return iDispatchJ;
    }

    private: 
    jobject iDispatchJ;
    int referenceCount;
};

_COM_SMARTPTR_TYPEDEF(JavaIDispatchAdapter, __uuidof(JavaIDispatchAdapter));

struct IDispatchJavaAdapter {
    IDispatchJavaAdapter(IDispatch* iDispatch) : 
        iDispatch(iDispatch),
        iDispatchJ(iDispatchToJava(iDispatch))
    {}

    ~IDispatchJavaAdapter() {
        if (iDispatchJ) releaseIDispatch(iDispatchJ);
    }

    private:
    IDispatchPtr iDispatch;
    jobject iDispatchJ;
};


extern "C"  // com.sos.engine.taskserver.windows.ScriptEngine#newScriptEngine
jobject JNICALL Java_com_sos_engine_taskserver_windows_scriptengine_ScriptEngine_newScriptEngine(JNIEnv* jniEnv, jclass, 
    jstring languageJ, jstring scriptJ, 
    jobject spoolerLogJ, jobject spoolerTaskJ, jobject spoolerJobJ, jobject spoolerJ)
{
    _bstr_t language = jstringToBstr(languageJ);
    _bstr_t script = jstringToBstr(scriptJ);
    JavaIDispatchAdapterPtr spoolerLog = new JavaIDispatchAdapter(spoolerLogJ);
    JavaIDispatchAdapterPtr spoolerTask = new JavaIDispatchAdapter(spoolerTaskJ);
    JavaIDispatchAdapterPtr spoolerJob = new JavaIDispatchAdapter(spoolerJobJ);
    JavaIDispatchAdapterPtr spooler = new JavaIDispatchAdapter(spoolerJ);
    return iDispatchToJava(newScriptEngine(language, script, spoolerLog, spoolerTask, spoolerJob, spooler));
}

static IDispatchPtr newScriptEngine(const _bstr_t& language, const _bstr_t& script, 
    const IDispatchPtr& spoolerLog, const IDispatchPtr& spoolerTask, const IDispatchPtr& spoolerJob, const IDispatchPtr& spooler) 
{
    return xxx;
}


/*
    newScriptEngine(name, script, spoolerLog, ...)

    spooler-Objekte in IDispatch-Adapter wickeln, 
    - der getIDsOfNames (-> idOfName) und Invoke weiterleitet
    - Invoke konvertiert Ein- und Rückgabeparameter
        - BSTR <-> jstring
        - IDispatch <-> IDispatch-Adapter
    - Release gibt frei

*/

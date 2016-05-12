#include <map>
#include <initguid.h>
#include <windows.h>
#include <unknwn.h>
#include <comdef.h>
#include <comdefsp.h>
#include <cguid.h>
#include <objbase.h>
#include <activscp.h>
#include "standards.h"

_COM_SMARTPTR_TYPEDEF(IActiveScript, __uuidof(IActiveScript));
_COM_SMARTPTR_TYPEDEF(IActiveScriptParse, __uuidof(IActiveScriptParse));

using namespace std;

CLSID stringToClsid(const _bstr_t& className) {
    return CLSID_NULL;
}

_bstr_t olecharToBstr(const OLECHAR* o) {
    return _bstr_t(o);
}

_bstr_t guidToBstr(const GUID& guid) {
    return _bstr_t("SOME GUID");
}

static bstr_t comExcepinfoException(HRESULT hresult, EXCEPINFO* excepinfo, const _bstr_t& function, const _bstr_t& insertion = "") {
    if (excepinfo->pfnDeferredFillIn) (*excepinfo->pfnDeferredFillIn)(excepinfo);

    HRESULT hr = excepinfo->scode ? excepinfo->scode : hresult;

    SysFreeString(excepinfo->bstrSource);         excepinfo->bstrSource = NULL;
    SysFreeString(excepinfo->bstrDescription);    excepinfo->bstrDescription = NULL;
    SysFreeString(excepinfo->bstrHelpFile);       excepinfo->bstrHelpFile = NULL;

    _bstr_t text = olecharToBstr(excepinfo->bstrSource) + ": " + olecharToBstr(excepinfo->bstrDescription);
    return comException(hr, function, insertion + " " + text);
}

struct ScriptEngineAdapter: IActiveScriptSite {
    ScriptEngineAdapter(const CLSID& clsid, const _bstr_t& script, 
        IDispatch* spoolerLog, IDispatch* spoolerTask, IDispatch* spoolerJob, IDispatch* spooler) 
    :
        iActiveScriptParse(newIActiveScriptParse(clsid)),
        spoolerLog(spoolerLog),
        spoolerTask(spoolerTask),
        spoolerJob(spoolerJob),
        spooler(spooler)
    {
        HRESULT hr;
        void* p;

        hr = iActiveScriptParse->InitNew();
        if (FAILED(hr)) throw comException(hr, "IActiveScriptParse::InitNew");

        hr = iActiveScriptParse->QueryInterface(IID_IActiveScript, (void**)&iActiveScript);
        if (FAILED(hr)) throw comException(hr, "IActiveScriptParse::QueryInterface");
        //iActiveScript.Attach((IActiveScript*)p);

        hr = iActiveScript->SetScriptSite(this);
        if (FAILED(hr)) throw comException(hr, "IActiveScript::SetScriptSite");

        addNamedItem(_bstr_t("spooler_log"), spoolerLog);
        addNamedItem(_bstr_t("spooler_task"), spoolerTask);
        addNamedItem(_bstr_t("spooler_job"), spoolerJob);
        addNamedItem(_bstr_t("spooler"), spooler);

        EXCEPINFO excepinfo;        
        memset( &excepinfo, 0, sizeof excepinfo );
        _bstr_t scriptBstr = script;
        if (!scriptBstr)  throw _bstr_t("Missing script for Script Engine"); // ParseScriptText may crash with empty BSTR

        variant_t parseResult = NULL;
        hr = iActiveScriptParse->ParseScriptText(scriptBstr, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISVISIBLE, &parseResult, &excepinfo);
        
        if (scriptException.length() > 0) {
            SysFreeString(excepinfo.bstrSource);
            SysFreeString(excepinfo.bstrDescription);
            SysFreeString(excepinfo.bstrHelpFile);
            throw scriptException;
        }

        if (FAILED(hr)) throw comExcepinfoException(hr, &excepinfo, "IActiveScriptParse::ParseScriptText");

        hr = iActiveScript->GetScriptDispatch(NULL, &iDispatch);
        if (FAILED(hr))  throw comException(hr, "IActiveScript::GetScriptDispatch");
        //iDispatch.Attach((IDispatch*)p);
    }

    private:
    static IActiveScriptParse* newIActiveScriptParse(const CLSID& clsid) {
        HRESULT hr;
        void* p = NULL;
        hr = CoGetClassObject(clsid, CLSCTX_ALL, NULL, IID_IClassFactory, (void**)&p);
        if (FAILED(hr))  throw comException(hr, "CoGetClassObject", guidToBstr(clsid));
        IClassFactoryPtr iClassFactory = (IClassFactory*)p;

        hr = iClassFactory->CreateInstance(NULL, IID_IActiveScriptParse, &p);
        if (FAILED(hr))  throw comException(hr, "IClassFactory::CreateInstance", _bstr_t("IID_IActiveScriptParse"));
        return (IActiveScriptParse*)p;
    }

    STDMETHODIMP_(HRESULT) QueryInterface(REFIID riid, void** ppvObject) {
        IUnknown* p = riid == IID_IUnknown || riid == IID_IActiveScriptSite? this : NULL;
        *ppvObject = p;
        if (!p) return E_NOINTERFACE;
        ((IUnknown*)*ppvObject)->AddRef();
        return NOERROR;
    };

    void addNamedItem(const _bstr_t& name, IDispatch* iDispatch) {
        HRESULT hr = iActiveScript->AddNamedItem(name, SCRIPTTEXT_ISVISIBLE);
        if (FAILED(hr)) throw comException(hr, "IActiveScript::AddNamedItem");
    }

    STDMETHODIMP_(HRESULT) OnScriptError(IActiveScriptError *iActiveScriptError) {
        DWORD dwCookie;
        LONG columnNr;
        ULONG lineNr;
        BSTR sourceLine = NULL;
        EXCEPINFO excepInfo;
        string description;

        memset(&excepInfo, 0, sizeof excepInfo);

        iActiveScriptError->GetSourcePosition(&dwCookie, &lineNr, &columnNr);
        iActiveScriptError->GetSourceLineText(&sourceLine);
        iActiveScriptError->GetExceptionInfo(&excepInfo);

        scriptException = comException(excepInfo.scode, "script", _bstr_t(excepInfo.bstrDescription, true) + ", in " + 
            _bstr_t(excepInfo.bstrSource, true) + ":" + intToBstr(lineNr) + ":" + intToBstr(columnNr) + ": " + _bstr_t(sourceLine, true));

        SysFreeString(sourceLine);
        SysFreeString(excepInfo.bstrSource);
        SysFreeString(excepInfo.bstrDescription);
        SysFreeString(excepInfo.bstrHelpFile);
        return E_FAIL;
    }

    private:
    IActiveScriptParsePtr const iActiveScriptParse;
    IActiveScriptPtr const iActiveScript;
    IDispatchPtr iDispatch;
    IDispatchPtr const spoolerLog;
    IDispatchPtr const spoolerTask;
    IDispatchPtr const spoolerJob;
    IDispatchPtr const spooler;
    _bstr_t scriptException;
};

#include "standards.h"
#include <string>

using namespace std;

bstr_t toHex8bstr(int value) {
    char buffer[8+1];
    sprintf_s(buffer, sizeof buffer, "%08X", value);
    return buffer;
}

inline _bstr_t intToBstr(int value) {
    char buffer[20];
    sprintf_s(buffer, sizeof buffer, "%d", value);
    return buffer;
}


bstr_t mswinException(int error, const _bstr_t& function, const bstr_t& insertion) {
    return _bstr_t("MSWIN-" + toHex8bstr(error) + " " + function + " ") + insertion;
}

bstr_t mswinException(const _bstr_t& function, const bstr_t& insertion) {
    return mswinException(GetLastError(), function, insertion);
}

bstr_t comException(HRESULT hr, const _bstr_t& function, const _bstr_t& insertion) {
    return bstr_t("COM-") + toHex8bstr(hr) + _bstr_t(" ") + function + " " + insertion;
}

int to_int(size_t n) {
    if (n > (size_t)INT_MAX) throw _bstr_t("Integer overflow");  // size_t ist unsigned, < 0 brauchen wir nicht zu prüfen.
    return (int)n;
}


BSTR string_to_bstr(const char* single_byte_text, size_t length_size_t) {
    int length = to_int(length_size_t);

    int count = MultiByteToWideChar(CP_ACP, 0, single_byte_text, length, NULL, 0);
    if (!count)  throw mswinException("bstr_from_string/MultiByteToWideChar");

    OLECHAR* ptr = SysAllocStringLen((OLECHAR*)NULL, count);
    if (!ptr)  throw comException(E_OUTOFMEMORY, "bstr_from_string/SysAllocStringLen");

    int c = MultiByteToWideChar(CP_ACP, 0, single_byte_text, length, ptr, length);
    if (!c)  throw mswinException("bstr_from_string/MultiByteToWideChar");

    ptr[count] = L'\0';
    return ptr;
}

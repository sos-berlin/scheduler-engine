#pragma once

#include <string>
#include <comdef.h>
#include <comutil.h>

extern int to_int(size_t);

//extern BSTR string_to_bstr(const char*, size_t);
//
//inline BSTR string_to_bstr(const std::string& s) {
//    return string_to_bstr(s.data(), s.length());
//}

extern _bstr_t intToBstr(int i);

extern _bstr_t comException(HRESULT hr, const _bstr_t& function, const _bstr_t& insertion = "");
extern _bstr_t mswinException(int error, const _bstr_t& function, const bstr_t& insertion = "");
extern _bstr_t mswinException(const bstr_t& function, const bstr_t& insertion = "");

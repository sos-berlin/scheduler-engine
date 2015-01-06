#ifndef __ZSCHIMMER_LOGIN_H__
#define __ZSCHIMMER_LOGIN_H__

namespace zschimmer {

struct Login : Object {
    private: string const _user_name;
    private: string const _password;

    public: Login(const string& name, const string& password) : _user_name(name), _password(password) {}
    public: const string& user_name() const { return _user_name; }
    public: const string& password() const { return _password; }
};

}

#endif

#pragma once

#include <Main.h>

namespace Balau {

class TLSManager {
  public:
    virtual void * getTLS();
    virtual void * setTLS(void * val);
    void * createTLS();
};

extern TLSManager * g_tlsManager;

class Local : public AtStart {
  public:
    static int getSize() { return s_size; }
  protected:
      Local() : AtStart(0) { }
    void * getGlobal() { return m_globals[m_idx]; }
    void * getLocal() { return reinterpret_cast<void **>(getTLS())[m_idx]; }
    void * get() { if (getTLS()) { void * l = getLocal(); return l ? l : getGlobal(); } else return getGlobal(); }
    void setGlobal(void * obj) { m_globals[m_idx] = obj; }
    void setLocal(void * obj) { void * r = getTLS(); reinterpret_cast<void **>(r)[m_idx] = obj; }
    void set(void * obj) { void * r = getTLS(); if (r) setLocal(obj); else setGlobal(obj); }
    int getIndex() { return m_idx; }
  private:
    static void * create() { void * r = calloc(s_size * sizeof(void *), 1); return r; }
    static void * getTLS() { return g_tlsManager->getTLS(); }
    static void * setTLS(void * val) { return g_tlsManager->setTLS(val); }
    virtual void doStart();
    int m_idx;
    static int s_size;
    static void ** m_globals;

    friend class TLSManager;
};

template<class T>
class DefaultTmpl : public AtStart {
  public:
      DefaultTmpl(int pri) : AtStart(pri) { }
  protected:
    virtual void doStart() { new T; }
};

template<class T>
class LocalTmpl : public Local {
  public:
      LocalTmpl() { }
    T * getGlobal() { return reinterpret_cast<T *>(Local::getGlobal()); }
    T * get() { return reinterpret_cast<T *>(Local::get()); }
    void setGlobal(T * obj) { Local::setGlobal(obj); }
    void set(T * obj) { Local::set(obj); }
};

};

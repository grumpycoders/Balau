#pragma once

#include <Main.h>

namespace Balau {

class TLSManager {
  public:
    virtual void * getTLS();
    virtual void * setTLS(void * val);
};

extern TLSManager * tlsManager;

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
    void set(void * obj) { void * r = getTLS(); if (r) setGlobal(obj); else setLocal(obj); }
    int getIndex() { return m_idx; }
  private:
    static void * create() { void * r = malloc(s_size * sizeof(void *)); setTLS(r); return r; }
    static void * getTLS() { return tlsManager->getTLS(); }
    static void * setTLS(void * val) { return tlsManager->setTLS(val); }
    virtual void doStart();
    int m_idx;
    static int s_size;
    static void ** m_globals;
};

};

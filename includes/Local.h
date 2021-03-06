#pragma once

#include <Main.h>

namespace Balau {

class TLSManager {
  public:
      TLSManager() { }
    virtual void * getTLS();
    virtual void * setTLS(void * val);
  private:
      TLSManager(const TLSManager &) = delete;
    TLSManager & operator=(const TLSManager &) = delete;
};

class PThreadsTLSManager : public TLSManager {
  public:
    virtual void * getTLS();
    virtual void * setTLS(void * val);
    void init();
  private:
    pthread_key_t m_key;
};

template <class TLS>
class PThreadsTLSFactory : private PThreadsTLSManager {
  public:
      PThreadsTLSFactory() : m_constructor([]() -> TLS * { return new TLS(); }) { init();  }
      ~PThreadsTLSFactory() { destroyAll(); }
    void setConstructor(const std::function<TLS *()> & constructor) { m_constructor = constructor; }
    TLS * get() {
        TLS * tls = (TLS *) getTLS();
        if (!tls) {
            tls = m_constructor();
            setTLS(tls);
            m_TLSes.push(tls);
            ++m_numTLSes;
        }
        return tls;
    }
  private:
    void destroyAll() {
        while (m_numTLSes--) {
            TLS * tls = m_TLSes.pop();
            delete tls;
        }
    }
    Queue<TLS> m_TLSes;
    std::atomic<unsigned> m_numTLSes;
    std::function<TLS *()> m_constructor;
};

extern TLSManager * g_tlsManager;

class Local : public AtStart {
  public:
    static int getSize() { return s_size; }
    static void * createTLS(void * c = NULL) {
        void * r = calloc(s_size * sizeof(void *), 1);
        if (c)
            memcpy(r, c, s_size * sizeof(void *));
        return r;
    }
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

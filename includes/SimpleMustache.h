#pragma once
#include <map>
#include <vector>
#include <list>
#include <Handle.h>
#include <Buffer.h>

namespace Balau {

class SimpleMustache {
  public:

    class Context {
      public:
        class Proxy {
          public:
            Context & operator[](const char * str);
          private:
              Proxy(Context * parent, ssize_t idx) : m_parent(parent), m_idx(idx) { }
            Context * m_parent;
            ssize_t m_idx;
            friend class Context;
        };

          Context() : m_type(CONTEXTLIST), m_root(true) { }
          ~Context() { empty(); }
        Proxy operator[](ssize_t idx) { ensureList(); return Proxy(this, idx); }
        Context & operator[](const char * str);
        // we should try and support lambdas, but I'm not entierely sure about them.
        // Something tells me they need some design love, especially about which
        // context they use. The specification says they should expand the tags,
        // but the example doesn't show a function that'd take a context...
        Context & operator=(const String & str) {
            empty();
            m_type = STRING;
            m_str = str;
            return *this;
        }
        Context & operator=(const char * str) {
            empty();
            m_type = STRING;
            m_str = str;
            return *this;
        }
        Context & operator=(bool b) {
            empty();
            m_type = BOOLSEC;
            m_bool = b;
            return *this;
        }
      private:
        enum ContextType {
            UNKNOWN,
            STRING,
            BOOLSEC,
            CONTEXTLIST,
            LAMBDA,
        } m_type;
          Context(ContextType type) : m_type(type), m_root(false) { }
          Context(Context & c) { Failure("You can't copy a Context; use references"); }
          Context & operator=(Context & c) { Failure("You can't assign a Context; use references"); return *this; }
        String m_str;
        bool m_bool;
        typedef std::map<String, Context *> SubContext;
        typedef std::vector<SubContext> ContextList;
        ContextList m_contextList;
        bool m_root;

        void empty(bool skipFirst = false);
        void ensureList(bool single = false);

        friend class Proxy;
        friend class SimpleMustache;
    };

    void setTemplate(IO<Handle> h);
    void setTemplate(const uint8_t * str, ssize_t s = -1) {
        if (s < 0)
            s = strlen((const char *) str);
        IO<Buffer> b(new Buffer(str, s));
        setTemplate(b);
    }
    template<size_t S>
    void setTemplate(const char (&str)[S]) { setTemplate((const uint8_t *) str, S - 1); }
    void setTemplate(const char * str, ssize_t s) { setTemplate((const uint8_t *) str, s); }
    void setTemplate(const String & str) { setTemplate((const uint8_t *) str.to_charp(), str.strlen()); }
    void render(IO<Handle> h, Context * ctx) const { AAssert(ctx, "Please pass on a context to render"); render_r(h, ctx, "", m_fragments.begin(), false, -1); }
    void empty() { while (!m_fragments.empty()) { delete m_fragments.front(); m_fragments.pop_front(); } }
    void checkTemplate() { Fragments::const_iterator end = checkTemplate_r(m_fragments.begin()); AAssert(end == m_fragments.end(), "The template wasn't fully checked; possibly mismatched sections"); }
      ~SimpleMustache() { empty(); }
  private:
    struct Fragment {
        enum {
            UNKNOWN,
            STRING,
            VARIABLE,
            NOESCAPE,
            SECTION,
            INVERTED,
            END_SECTION,
        } type;
        String str; // contains either the string, the variable name, or the sections names.
    };
    typedef std::list<Fragment *> Fragments;
    Fragments m_fragments;

    Fragments::const_iterator render_r(IO<Handle> h, Context * ctx, const String & endSection, Fragments::const_iterator begin, bool noWrite, int forceIdx) const;
    static String escape(const String & s);

    Fragments::const_iterator checkTemplate_r(Fragments::const_iterator begin, const String & endSection = "") const;
};

};

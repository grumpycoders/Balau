#include "SimpleMustache.h"
#include "BStream.h"
#include "BRegex.h"

/* Example of use


    SimpleMustache tpl;
    const char tplStr[] =
"<h1>{{header}}</h1>\n"
"{{#bug}}\n"
"{{/bug}}\n"
"\n"
"{{#items}}\n"
"  {{#first}}\n"
"    <li><strong>{{name}}</strong></li>\n"
"  {{/first}}\n"
"  {{#link}}\n"
"    <li><a href=\"{{url}}\">{{name}}</a></li>\n"
"  {{/link}}\n"
"{{/items}}\n"
"\n"
"{{#empty}}\n"
"  <p>The list is empty.</p>\n"
"{{/empty}}\n";
    tpl.setTemplate(tplStr, sizeof(tplStr) - 1);
    SimpleMustache::Context ctx;
    ctx["header"] = "Colors";
    ctx["items"][0]["name"] = "red";
    ctx["items"][-1]["first"] = true;
    ctx["items"][-1]["url"] = "#Red";
    ctx["items"][0]["name"] = "green";
    ctx["items"][-1]["link"] = true;
    ctx["items"][-1]["url"] = "#Green";
    ctx["items"][0]["name"] = "blue";
    ctx["items"][-1]["link"] = true;
    ctx["items"][-1]["url"] = "#Blue";
    ctx["empty"] = false;

    IO<HPrinter> b(new HPrinter);
    tpl.render(b, &ctx);



will output:

<h1>Colors</h1>
    <li><strong>red</strong></li>
    <li><a href="#Green">green</a></li>
    <li><a href="#Blue">blue</a></li>

(and a few extra blank lines)

indexes for contextes are counting from 1
index  0 of a context == new slot
index -x of a context == slot number size - x

*/

Balau::SimpleMustache::Context & Balau::SimpleMustache::Context::Proxy::operator[](const char * str) {
    Assert(m_parent->m_type == CONTEXTLIST);
    String key = str;
    ContextList & ctxLst = m_parent->m_contextList;
    if (m_idx <= 0)
        m_idx = ctxLst.size() + m_idx + 1;
    if (m_idx <= 0)
        m_idx = 1;
    if (ctxLst.size() < m_idx)
        ctxLst.resize(m_idx);
    SubContext & subCtx = ctxLst[m_idx - 1];
    SubContext::iterator s = subCtx.find(key);
    Context * ctx;
    if (s == subCtx.end()) {
        ctx = subCtx[key] = new Context(UNKNOWN);
    } else {
        ctx = s->second;
    }
    return *ctx;
}

Balau::SimpleMustache::Context & Balau::SimpleMustache::Context::operator[](const char * str) {
    ensureList(true);
    String key = str;
    SubContext & sCtx = m_contextList[0];
    SubContext::iterator s = sCtx.find(str);
    Context * ctx;
    if (s == sCtx.end()) {
        ctx = sCtx[key] = new Context(UNKNOWN);
    } else {
        ctx = s->second;
    }
    return *ctx;
}

void Balau::SimpleMustache::Context::ensureList(bool single) {
    if (m_type == CONTEXTLIST) {
        if (single)
            empty(true);
    } else {
        empty();
        m_type = CONTEXTLIST;
    }
}

void Balau::SimpleMustache::Context::empty(bool skipFirst) {
    for (ContextList::iterator i = m_contextList.begin(); i != m_contextList.end(); i++) {
        if (skipFirst && i == m_contextList.begin())
            continue;
        for (SubContext::iterator j = i->begin(); j != i->end(); j++) {
            delete j->second;
        }
    }
    m_contextList.resize(skipFirst ? 1 : 0);
}

static const Balau::Regex changing("^(.*) +(.*)$");

void Balau::SimpleMustache::setTemplate(IO<Handle> _h) {
    empty();
    IO<BStream> h(new BStream(_h));
    h->detach();
    String srtMarker = "{{";
    String endMarker = "}}";
    enum {
        PLAIN,
        READING_SRT,
        READING_INNER,
        READING_END,
    } state = PLAIN;
    enum {
        NORMAL,
        SECTION,
        NOESCAPE,
        ENDSECTION,
        INVERTED,
        PARTIAL,
        CHANGING,
        COMMENT,
    } tagType = NORMAL;
    int dist = 0;
    char buf[16 * 1024 + 1];
    char * p = buf;
    static const int bufLen = 16 * 1024;
    Fragment * curFragment = new Fragment();
    curFragment->type = Fragment::STRING;
    bool stupidMarker = false;
    bool readFirstEnd = false;
    bool beginning = false;

    while (!h->isEOF()) {
        uint8_t c = h->readU8();
        switch (state) {
        case PLAIN:
            if (c != srtMarker[0]) {
                *p++ = static_cast<char>(c);
                if (p == (buf + bufLen)) {
                    *p = 0;
                    curFragment->str += String(buf, p - buf);
                    p = buf;
                }
                break;
            } else {
                if (p != buf) {
                    *p = 0;
                    curFragment->str += String(buf, p - buf);
                    p = buf;
                }
                state = READING_SRT;
                dist = 0;
            }
        case READING_SRT:
            if (c != srtMarker[dist]) {
                curFragment->str += srtMarker.extract(0, dist) + String((const char *) &c, 1);
                state = PLAIN;
            } else {
                if (++dist == srtMarker.strlen()) {
                    if (curFragment->str.strlen() != 0) {
                        m_fragments.push_back(curFragment);
                        curFragment = new Fragment();
                    }
                    curFragment->type = Fragment::UNKNOWN;
                    state = READING_INNER;
                    beginning = true;
                }
            }
            break;
        case READING_INNER:
            if (beginning) {
                Assert(p == buf);
                beginning = false;
                tagType = NORMAL;
                stupidMarker = false;
                bool skip = true;
                switch (c) {
                case '#':
                    tagType = SECTION;
                    break;
                case '&':
                    tagType = NOESCAPE;
                    break;
                case '/':
                    tagType = ENDSECTION;
                    break;
                case '^':
                    tagType = INVERTED;
                    break;
                case '>':
                    tagType = PARTIAL;
                    break;
                case '=':
                    tagType = CHANGING;
                    break;
                case '!':
                    tagType = COMMENT;
                    break;
                case '{':
                    if (srtMarker == "{{") {
                        stupidMarker = true;
                        tagType = NOESCAPE;
                        break;
                    }
                default:
                    skip = false;
                    break;
                }
                if (skip)
                    continue;
            }
            if (c != endMarker[0]) {
                *p++ = static_cast<char>(c);
                if (p == (buf + bufLen)) {
                    *p = 0;
                    curFragment->str += String(buf, p - buf);
                    p = buf;
                }
                break;
            } else {
                if (p != buf) {
                    *p = 0;
                    curFragment->str += String(buf, p - buf);
                    p = buf;
                }
                state = READING_END;
                readFirstEnd = true;
                dist = 0;
            }
        case READING_END:
            if (c != endMarker[dist]) {
                curFragment->str += endMarker.extract(0, dist) + String((const char *) &c, 1);
                state = READING_INNER;
            } else {
                if (stupidMarker && readFirstEnd) {
                    readFirstEnd = 0;
                    dist--;
                }
                if (++dist == endMarker.strlen()) {
                    bool pushIt = true;
                    String str = curFragment->str;
                    Assert(str.strlen() != 0);
                    Assert(tagType != PARTIAL); // not yet supported
                    Regex::Captures c;
                    switch (tagType) {
                    case NORMAL:
                        curFragment->type = Fragment::VARIABLE;
                        break;
                    case SECTION:
                        curFragment->type = Fragment::SECTION;
                        break;
                    case NOESCAPE:
                        curFragment->type = Fragment::NOESCAPE;
                        break;
                    case ENDSECTION:
                        curFragment->type = Fragment::END_SECTION;
                        break;
                    case INVERTED:
                        curFragment->type = Fragment::INVERTED;
                        break;
                    case PARTIAL:
                        Assert(0);
                        break;
                    case CHANGING:
                        pushIt = false;
                        Assert(str[0] == '=');
                        Assert(str[-1] == '=');
                        c = changing.match(str.to_charp());
                        Assert(c.size() == 3);
                        srtMarker = c[1];
                        endMarker = c[2];
                        Assert(srtMarker.strlen() != 0);
                        Assert(endMarker.strlen() != 0);
                        Assert(srtMarker[0] != endMarker[0]);
                        Assert(srtMarker.strchr(' ') < 0);
                        Assert(srtMarker.strchr('=') < 0);
                        Assert(endMarker.strchr(' ') < 0);
                        Assert(endMarker.strchr('=') < 0);
                        break;
                    case COMMENT:
                        pushIt = false;
                        break;
                    }
                    if (pushIt) {
                        Assert(curFragment->type != Fragment::UNKNOWN);
                        m_fragments.push_back(curFragment);
                        curFragment = new Fragment();
                    }
                    curFragment->type = Fragment::STRING;
                    curFragment->str = "";
                    state = PLAIN;
                }
            }
            break;
        }
    }

    Assert(state == PLAIN);

    if (p != buf) {
        *p = 0;
        curFragment->str += String(buf, p - buf);
    }

    if (curFragment->str.strlen() == 0)
        delete curFragment;
    else
        m_fragments.push_back(curFragment);
}

Balau::SimpleMustache::Fragments::iterator Balau::SimpleMustache::render_r(IO<Handle> h, Context * ctx, const String & endSection, Fragments::iterator begin, bool noWrite, int forceIdx) {
    Fragments::iterator cur;
    Fragments::iterator end = m_fragments.end();

    if (endSection.strlen() != 0) {
        int depth = 0;
        for (cur = begin; cur != end; cur++) {
            Fragment * fr = *cur;
            if (fr->type == Fragment::END_SECTION) {
                if (depth == 0) {
                    Assert(fr->str == endSection);
                    end = cur;
                    break;
                } else {
                    depth--;
                }
            }
            if ((fr->type == Fragment::SECTION) || (fr->type == Fragment::INVERTED))
                depth++;
        }
        Assert(end != m_fragments.end());
    }

    if (!ctx) {
        if (noWrite)
            return end;
        for (cur = begin; cur != end; cur++) {
            Fragment * fr = *cur;
            if(fr->type == Fragment::STRING)
                h->write(fr->str);
        }
        return end;
    }

    Assert(!noWrite);

    Context::ContextList::iterator sCtx;
    int idx = 0;
    for (sCtx = ctx->m_contextList.begin(); sCtx != ctx->m_contextList.end(); sCtx++, idx++) {
        if ((forceIdx >= 0) && (idx != forceIdx))
            continue;
        Context::SubContext::iterator f;
        for (cur = begin; cur != end; cur++) {
            Fragment * fr = *cur;
            Assert(fr->type != Fragment::UNKNOWN);
            Assert(fr->type != Fragment::END_SECTION);
            switch (fr->type) {
            case Fragment::STRING:
                h->write(fr->str);
                break;
            case Fragment::VARIABLE:
                f = sCtx->find(fr->str);
                if (f != sCtx->end()) {
                    Context * var = f->second;
                    Assert(var->m_type == Context::STRING);
                    h->write(escape(var->m_str));
                }
                break;
            case Fragment::NOESCAPE:
                f = sCtx->find(fr->str);
                if (f != sCtx->end()) {
                    Context * var = f->second;
                    Assert(var->m_type == Context::STRING);
                    h->write(var->m_str);
                }
                break;
            case Fragment::SECTION:
                f = sCtx->find(fr->str);
                if (f != sCtx->end()) {
                    Context * var = f->second;
                    if (var->m_type == Context::CONTEXTLIST) {
                        cur = render_r(h, var, fr->str, ++cur, false, -1);
                        break;
                    } else if (((var->m_type == Context::BOOLSEC) && (var->m_bool)) || (var->m_type != Context::BOOLSEC)) {
                        cur = render_r(h, ctx, fr->str, ++cur, false, idx);
                        break;
                    }
                }
                cur = render_r(h, NULL, fr->str, ++cur, true, -1);
                break;
            case Fragment::INVERTED:
                cur = render_r(h, NULL, fr->str, ++cur, sCtx->find(fr->str) != sCtx->end(), -1);
                break;
            default:
                Assert(false);
                break;
            }
        }
    }

    return end;
}

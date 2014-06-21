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
    IAssert(m_parent->m_type == CONTEXTLIST, "We got a [str] request on a ContextProxy which parent isn't a CONTEXTLIST... ?");
    String key = str;
    ContextList & ctxLst = m_parent->m_contextList;
    if (m_idx <= 0)
        m_idx = ctxLst.size() + m_idx + 1;
    if (m_idx <= 0)
        m_idx = 1;
    if (ctxLst.size() < static_cast<size_t>(m_idx))
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
    Task::SimpleContext simpleContext;
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
        uint8_t c = h->readU8().get();
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
                IAssert(p == buf, "READING_INNER; beginning = true but p isn't at the beginning of the buffer...");
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
                    AAssert(str.strlen() != 0, "Got an empty tag... ?");
                    AAssert(tagType != PARTIAL, "Partials aren't supported yet");
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
                        // note: it'd be a nice optimization here to remember and find the
                        // locations of the start section, so to point the start section
                        // at its end, to avoid useless loops in the renderer.
                        curFragment->type = Fragment::END_SECTION;
                        break;
                    case INVERTED:
                        curFragment->type = Fragment::INVERTED;
                        break;
                    case PARTIAL:
                        Failure("Partials aren't supported yet");
                        break;
                    case CHANGING:
                        pushIt = false;
                        IAssert(str[0] == '=', "A CHANGING tag that doesn't start with =... ?");
                        AAssert(str[-1] == '=', "A changing tag must end with =");
                        c = changing.match(str.to_charp());
                        IAssert(c.size() == 3, "The 'changing' regexp didn't match...");
                        srtMarker = c[1];
                        endMarker = c[2];
                        AAssert(srtMarker.strlen() != 0, "A new Mustache marker can't be empty.");
                        AAssert(endMarker.strlen() != 0, "A new Mustache marker can't be empty.");
                        AAssert(srtMarker[0] != endMarker[0], "The beginning and end markers can't start with the same character");
                        AAssert(srtMarker.strchr(' ') < 0, "A mustache marker can't contain spaces");
                        AAssert(srtMarker.strchr('=') < 0, "A mustache marker can't contain '='");
                        AAssert(endMarker.strchr(' ') < 0, "A mustache marker can't contain spaces");
                        AAssert(endMarker.strchr('=') < 0, "A mustache marker can't contain '='");
                        break;
                    case COMMENT:
                        pushIt = false;
                        break;
                    }
                    if (pushIt) {
                        IAssert(curFragment->type != Fragment::UNKNOWN, "We got an unknown fragment at that point...?");
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

    IAssert(state == PLAIN, "We shouldn't exit that parsing loop without being in the 'PLAIN' state");

    if (p != buf) {
        *p = 0;
        curFragment->str += String(buf, p - buf);
    }

    if (curFragment->str.strlen() == 0)
        delete curFragment;
    else
        m_fragments.push_back(curFragment);
}

Balau::SimpleMustache::Fragments::const_iterator Balau::SimpleMustache::checkTemplate_r(Fragments::const_iterator begin, const String & endSection) const {
    Fragments::const_iterator cur;
    Fragments::const_iterator end = m_fragments.end();

    for (cur = begin; cur != end; cur++) {
        Fragment * fr = *cur;
        if ((fr->type == Fragment::END_SECTION) && (endSection.strlen() != 0)) {
            AAssert(fr->str == endSection, "Beginning / End sections mismatch (%s != %s)", fr->str.to_charp(), endSection.to_charp());
            return cur;
        }
        AAssert(fr->type != Fragment::END_SECTION, "Reached an extra end section (%s)", fr->str.to_charp());
        if ((fr->type == Fragment::SECTION) || (fr->type == Fragment::INVERTED))
            cur = checkTemplate_r(++cur, fr->str);
    }

    return end;
}

Balau::SimpleMustache::Fragments::const_iterator Balau::SimpleMustache::render_r(IO<Handle> h, Context * ctx, const String & endSection, Fragments::const_iterator begin, bool noWrite, int forceIdx) const {
    Fragments::const_iterator cur;
    Fragments::const_iterator end = m_fragments.end();

    if (endSection.strlen() != 0) {
        int depth = 0;
        for (cur = begin; cur != end; cur++) {
            Fragment * fr = *cur;
            if (fr->type == Fragment::END_SECTION) {
                if (depth == 0) {
                    IAssert(fr->str == endSection, "Beginning / End sections mismatch (%s != %s); shouldn't have checkTemplate caught that... ?", fr->str.to_charp(), endSection.to_charp());
                    end = cur;
                    break;
                } else {
                    depth--;
                }
            }
            if ((fr->type == Fragment::SECTION) || (fr->type == Fragment::INVERTED))
                depth++;
        }
        IAssert(end != m_fragments.end(), "Reached end of template without finding an end section for %s; shouldn't have checkTemplate caught that... ?", endSection.to_charp());
    }

    if (!ctx) {
        if (noWrite)
            return end;
        for (cur = begin; cur != end; cur++) {
            Fragment * fr = *cur;
            if(fr->type == Fragment::STRING)
                h->writeString(fr->str);
        }
        return end;
    }

    IAssert(!noWrite, "noWrite == true but we have a context... ?");

    Context::ContextList::iterator sCtx;
    int idx = 0;
    for (sCtx = ctx->m_contextList.begin(); sCtx != ctx->m_contextList.end(); sCtx++, idx++) {
        if ((forceIdx >= 0) && (idx != forceIdx))
            continue;
        Context::SubContext::iterator f;
        for (cur = begin; cur != end; cur++) {
            Fragment * fr = *cur;
            IAssert(fr->type != Fragment::UNKNOWN, "Processing an unknown fragment... ?");
            IAssert(fr->type != Fragment::END_SECTION, "Processing an end section tag... ?");
            switch (fr->type) {
            case Fragment::STRING:
                h->writeString(fr->str);
                break;
            case Fragment::VARIABLE:
                f = sCtx->find(fr->str);
                if (f != sCtx->end()) {
                    Context * var = f->second;
                    if (var->m_type == Context::STRING)
                        h->writeString(escape(var->m_str));
                    else if (var->m_type == Context::BOOLSEC)
                        h->writeString(var->m_bool ? "true" : "false");
                }
                break;
            case Fragment::NOESCAPE:
                f = sCtx->find(fr->str);
                if (f != sCtx->end()) {
                    Context * var = f->second;
                    if (var->m_type == Context::STRING)
                        h->writeString(var->m_str);
                    else if (var->m_type == Context::BOOLSEC)
                        h->writeString(var->m_bool ? "true" : "false");
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
                FailureDetails("We shouldn't end up here", "fragment type = %i", fr->type);
                break;
            }
        }
    }

    return end;
}

Balau::String Balau::SimpleMustache::escape(const String & s) {
    int size = 0;

    for (unsigned i = 0; i < s.strlen(); i++) {
        switch (s[i]) {
        case '&':
            size += 5;
            break;
        case '"':
            size += 6;
            break;
        case '\'':
            size += 5;
            break;
        case '\\':
            size += 5;
            break;
        case '<':
            size += 4;
            break;
        case '>':
            size += 4;
            break;
        default:
            size++;
            break;
        }
    }

    char * t = (char *) malloc(size + 1);
    char * p = t;

    for (unsigned i = 0; i < s.strlen(); i++) {
        switch (s[i]) {
        case '&':
            *p++ = '&';
            *p++ = 'a';
            *p++ = 'm';
            *p++ = 'p';
            *p++ = ';';
            break;
        case '"':
            *p++ = '&';
            *p++ = 'q';
            *p++ = 'u';
            *p++ = 'o';
            *p++ = 't';
            *p++ = ';';
            break;
        case '\'':
            *p++ = '&';
            *p++ = '#';
            *p++ = '3';
            *p++ = '9';
            *p++ = ';';
            break;
        case '\\':
            *p++ = '&';
            *p++ = '#';
            *p++ = '9';
            *p++ = '2';
            *p++ = ';';
            break;
        case '<':
            *p++ = '&';
            *p++ = 'l';
            *p++ = 't';
            *p++ = ';';
            break;
        case '>':
            *p++ = '&';
            *p++ = 'g';
            *p++ = 't';
            *p++ = ';';
            break;
        default:
            *p++ = s[i];
            break;
        }
    }

    *p = 0;

    String r(t, size);
    free(t);
    return r;
}

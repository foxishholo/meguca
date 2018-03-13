#include "mutations.hh"
#include <emscripten.h>

namespace brunhild {
using std::string;

void append(unsigned long id, string html)
{
    EM_ASM_INT(
        {
            var el = document.getElementById('bh-' + $0);
            var cont = document.createElement('div');
            cont.innerHTML = UTF8ToString($1);
            el.appendChild(cont.firstChild);
        },
        id, html.data());
}

void prepend(unsigned long id, string html)
{
    EM_ASM_INT(
        {
            var el = document.getElementById('bh-' + $0);
            var cont = document.createElement('div');
            cont.innerHTML = UTF8ToString($1);
            el.insertBefore(cont.firstChild, el.firstChild);
        },
        id, html.c_str());
}

void before(unsigned long id, string html)
{
    EM_ASM_INT(
        {
            var el = document.getElementById('bh-' + $0);
            var cont = document.createElement('div');
            cont.innerHTML = UTF8ToString($1);
            el.parentNode.insertBefore(cont.firstChild, el);
        },
        id, html.c_str());
}

void after(unsigned long id, string html)
{
    EM_ASM_INT(
        {
            var el = document.getElementById('bh-' + $0);
            var cont = document.createElement('div');
            cont.innerHTML = UTF8ToString($1);
            el.parentNode.insertBefore(cont.firstChild, el.nextSibling);
        },
        id, html.c_str());
}

void set_inner_html(unsigned long id, string html)
{
    EM_ASM_INT(
        { document.getElementById('bh-' + $0).innerHTML = UTF8ToString($1); },
        id, html.c_str());
}

void set_outer_html(unsigned long id, string html)
{
    EM_ASM_INT(
        { document.getElementById('bh-' + $0).outerHTML = UTF8ToString($1); },
        id, html.c_str());
}

void remove(unsigned long id)
{
    EM_ASM(
        {
            var el = document.getElementById('bh-' + $0);
            el.parentNode.removeChild(el);
        },
        id);
}

void set_attr(unsigned long id, string key, string val)
{
    EM_ASM_INT(
        {
            document.getElementById('bh-' + $0)
                .setAttribute(UTF8ToString($1), UTF8ToString($2));
        },
        id, key.c_str(), val.c_str());
}

void remove_attr(unsigned long id, string key)
{
    EM_ASM_INT(
        {
            document.getElementById('bh-' + $0)
                .removeAttribute(UTF8ToString($1));
        },
        id, key.c_str());
}

void scroll_into_view(unsigned long id)
{
    EM_ASM_INT({ document.getElementById('bh-' + $0).scrollIntoView(); }, id);
}
}

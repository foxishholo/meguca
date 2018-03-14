#include "view.hh"
#include "../src/util.hh"
#include "mutations.hh"
#include "tree.hh"

using std::move;
using std::string;

// Zero is the ID assigned to <body>
static unsigned long id_counter = 1;

namespace brunhild {

std::string escape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() * 1.1);
    for (auto ch : s) {
        switch (ch) {
        case '&':
            out += "&amp;";
            break;
        case '\'':
            out += "&#39;"; // "&#39;" is shorter than "&apos;"
            break;
        case '<':
            out += "&lt;";
            break;
        case '>':
            out += "&gt;";
            break;
        case '\"':
            out += "&#34;"; // "&#34;" is shorter than "&quot;"
            break;
        default:
            out += ch;
        }
    }
    return out;
}

Node::Node(std::string tag, Attrs attrs, std::vector<Node> children)
    : tag(tag)
    , attrs(attrs)
    , children(children)
{
}

Node::Node(std::string tag, Attrs attrs, std::string html, bool escape)
    : tag(tag)
    , attrs(attrs)
    , inner_html(escape ? brunhild::escape(html) : html)
{
}

Node::Node(std::string tag, std::string html, bool escape)
    : Node(tag, {}, html, escape)
{
}

Children::Children(std::vector<std::shared_ptr<View>> children)
    : type(Type::views)
    , views(children)
{
}

Children::Children(std::vector<Node> children)
    : type(Type::nodes)
    , nodes(children)
{
}

Children::Children(std::string html, bool escape)
    : type(Type::html)
    , html(escape ? brunhild::escape(html) : html)
{
}

View::View(std::string tag, bool is_const)
    : is_const(is_const)
    , id(id_counter++)
    , tag(tag)
{
}
}

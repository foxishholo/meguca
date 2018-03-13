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

Node::Node(std::string tag, Attrs attrs = {}, std::vector<Node> children = {})
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

std::string Node::html() const
{
    std::ostringstream s;
    write_html(s);
    return s.str();
}

void Node::write_html(std::ostringstream& s) const
{
    s << '<' << tag;
    for (auto & [ key, val ] : attrs) {
        s << ' ' << key;
        if (val != "") {
            s << "=\"" << val << '"';
        }
    }
    s << '>';

    // These should be left empty and unterminated
    if (tag == "br" || tag == "wbr") {
        return;
    }

    if (inner_html) {
        s << *inner_html;
    } else {
        for (auto& ch : children) {
            ch.write_html(s);
        }
    }

    s << "</" << tag << '>';
}

Children::Children(std::vector<std::shared_ptr<View>> children)
    : type(ChildType::views)
    , ch_views(children)
{
}

Children::Children(std::vector<Node> children)
    : type(ChildType::nodes)
    , ch_nodes(children)
{
}

Children::Children(std::string html, escape)
    : type(ChildType::html)
    , html(escape ? brunhild::escape(html) : html)
{
}

View::View(std::string tag, bool is_const)
    : tag(tag)
    , is_parent(is_parent)
    , is_const(is_const)
    , id(id_counter++)
{
}
}

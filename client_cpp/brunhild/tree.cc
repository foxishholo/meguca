#include "tree.hh"
#include "mutations.hh"
#include "view.hh"
#include <emscripten.h>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace brunhild {
void (*before_flush)() = nullptr;
void (*after_flush)() = nullptr;

// Zero is the ID assigned to <body>
static unsigned long id_counter = 1;

// Keeps track of views to be patched, so there is no need to keep traversing
// the tree, if all views are already patched.
static std::unordered_set<unsigned long> to_patch;

// ID of View to scroll to after next render
static unsigned long scroll_to = 0;

// Root of the document <body> as a tree of Views
static Children view_root;

// Node representing the status of a DOM node since the last diff
struct DOMNode {
    unsigned long id;
    std::string tag;
    Attrs attrs;
    struct Children {
        std::vector<DOMNode> nodes;
        std::optional<std::string> html;

        Children() = default;

        // Constructs DOMNode subtree from View or Node subtree
        Children(brunhild::Children&& ch)
        {
            switch (ch.type) {
            case brunhild::Children::Type::views:
                nodes.resize(ch.views.size());
                for (size_t i = 0; i < ch.views.size(); i++) {
                    nodes[i] = { ch.views[i].get() };
                }
                html = std::nullopt;
                break;
            case brunhild::Children::Type::nodes:
                nodes.resize(ch.nodes.size());
                for (size_t i = 0; i < nodes.size(); i++) {
                    nodes[i] = { ch.nodes[i] };
                }
                html = std::nullopt;
                break;
            case brunhild::Children::Type::html:
                html = ch.html;
                nodes.clear();
                break;
            }
        }
    } children;

    DOMNode() = default;

    // Construct DOM tree from View
    DOMNode(View* view)
        : id(view->id)
        , tag(view->tag)
        , attrs(view->attrs())
        , children(view->children())
    {
    }

    // Construct DOM tree from Node
    DOMNode(Node node)
        : id(new_id())
        , tag(node.tag)
        , attrs(node.attrs)
        , children(node.children)
    {
    }

    // Write node and subtree to HTML
    std::string html() const
    {
        std::ostringstream s;
        write_html(s);
        return s.str();
    }

    void write_html(std::ostringstream& s) const
    {
        s << '<' << tag << " id=\"" << id << '"';
        for (auto & [ key, val ] : attrs) {
            if (key == "id") {
                continue;
            }
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

        if (children.html) {
            s << *children.html;
        } else {
            for (auto& ch : children.nodes) {
                ch.write_html(s);
            }
        }

        s << "</" << tag << '>';
    }
};

// Tree representing the current state of the DOM
static DOMNode::Children DOM_root;

// Generate a new unique Element ID
unsigned long new_id() { return id_counter++; }

void set_body_children(Children ch) { view_root = ch; }

void View::patch() { to_patch.insert(id); }

void View::scroll_into_view() { scroll_to = id; }

static void diff_attrs(DOMNode& dom_node, Attrs&& attrs)
{
    // Attributes added or changed
    for (auto & [ key, val ] : attrs) {
        if (key != "id" && dom_node.attrs[key] != val) {
            dom_node.attrs[key] = val;
            set_attr(dom_node.id, key, val);
        }
    }

    // Attributes removed
    for (auto & [ key, _ ] : dom_node.attrs) {
        if (!attrs.count(key)) {
            dom_node.attrs.erase(key);
            remove_attr(dom_node.id, key);
        }
    }
}

static void diff_node(DOMNode& dom_node, View* view);
static void diff_node(DOMNode& dom_node, Node&& node);

static void diff_children(
    DOMNode& dom_node, std::vector<std::shared_ptr<View>>&& views)
{
    // First check, if any mutation in child order has ocurred
    bool need_diff = dom_node.children.nodes.size() != views.size();
    if (!need_diff) {
        for (size_t i = 0; i < dom_node.children.nodes.size(); i++) {
            if (dom_node.children.nodes[i].id != views[i]->id) {
                need_diff = true;
                break;
            }
        }
    }

    // No mutation
    if (!need_diff) {
        for (size_t i = 0; i < dom_node.children.nodes.size(); i++) {
            diff_node(dom_node.children.nodes[i], views[i].get());
        }
        return;
    }

    // Reorder nodes
    std::unordered_map<unsigned long, DOMNode*> existing;
    existing.reserve(dom_node.children.nodes.size());
    for (auto& ch : dom_node.children.nodes) {
        existing[ch.id] = &ch;
    }
    std::vector<DOMNode> new_children;
    new_children.reserve(views.size());
    unsigned long last_id = 0;
    for (auto& view : views) {
        auto v = view.get();
        DOMNode new_node;
        if (!existing.count(v->id)) {
            new_node = { v };
            if (last_id) {
                after(last_id, new_node.html());
            } else {
                prepend(dom_node.id, new_node.html());
            }
        } else {
            new_node = *existing[v->id];
            existing.erase(v->id);
            diff_node(new_node, v);
        }
        new_children.push_back(new_node);

        last_id = v->id;
    }

    // Remove any unused nodes
    for (auto & [ id, _ ] : existing) {
        remove(id);
    }

    dom_node.children.nodes = new_children;
}

static void diff_children(DOMNode& dom_node, std::vector<Node>&& nodes)
{
    // Diff existing nodes
    for (size_t i = 0; i < dom_node.children.nodes.size() && i < nodes.size();
         i++) {
        diff_node(dom_node.children.nodes[i], std::move(nodes[i]));
    }

    int diff = (int)nodes.size() - (int)dom_node.children.nodes.size();
    if (diff > 0) { // Append nodes
        size_t i = dom_node.children.nodes.size();
        while (i < nodes.size()) {
            DOMNode new_node = { nodes[i++] };
            append(dom_node.id, new_node.html());
            dom_node.children.nodes.push_back(new_node);
        }
    } else { // Remove Nodes from the end
        while (diff++ < 0) {
            remove(dom_node.id);
            dom_node.children.nodes.pop_back();
        }
    }
}

static void clear_inner_html(DOMNode& dom_node)
{
    if (dom_node.children.html) {
        set_inner_html(dom_node.id, "");
        dom_node.children.html = std::nullopt;
    }
}

static void diff_children(DOMNode& dom_node, Children&& children)
{
    switch (children.type) {
    case Children::Type::views:
        // Override possible previous conflicting state
        clear_inner_html(dom_node);
        diff_children(dom_node, children.views);
        break;
    case Children::Type::nodes:
        clear_inner_html(dom_node);
        diff_children(dom_node, children.nodes);
        dom_node.children.html = std::nullopt;
        break;
    case Children::Type::html:
        if (dom_node.children.html != children.html) {
            set_inner_html(dom_node.id, children.html);
            dom_node.children.html = children.html;
        }
        dom_node.children.nodes.clear();
        break;
    }
}

// Replace entire DOMNode with new one generated from src
template <class T> static void replace_dom_node(DOMNode& dom_node, T src)
{
    const auto old_id = dom_node.id;
    dom_node = { src };
    set_outer_html(old_id, dom_node.html());
}

static void diff_node(DOMNode& dom_node, Node&& node)
{
    // Completely replace node and subtree
    if (dom_node.tag != node.tag) {
        return replace_dom_node(dom_node, node);
    }
    diff_attrs(dom_node, std::move(node.attrs));
    diff_children(dom_node, node.children);
}

static void diff_node(DOMNode& dom_node, View* view)
{
    if (view->is_const) {
        return;
    }
    if (dom_node.id != view->id) {
        return replace_dom_node(dom_node, view);
    }
    diff_attrs(dom_node, view->attrs());
    diff_children(dom_node, view->children());
    to_patch.erase(view->id);
}

// Descend tree and find all views marked as needing a patch. Template, because
// T can be either an lvalue or an rvalue.
template <class T>
static void find_marked(DOMNode::Children& dom_children, T children)
{
    // Only Views can be marked as needing a patch. Rest can be ignored.
    if (children.type == Children::Type::views) {
        for (size_t i = 0; i < children.views.size(); i++) {
            auto v = children.views[i].get();
            if (v->is_const) {
                continue;
            }
            auto& node = dom_children.nodes[i];
            if (to_patch.count(v->id)) {
                diff_node(node, v);
            } else {
                find_marked(node.children, v->children());
            }

            // All marked nodes patched
            if (to_patch.empty()) {
                return;
            }
        }
    }
}

extern "C" void brunhild_diff_DOM()
{
    if (before_flush) {
        (*before_flush)();
    }
    if (!to_patch.empty()) {
        find_marked(DOM_root, view_root);
    }
    if (scroll_to) {
        scroll_into_view(scroll_to);
        scroll_to = 0;
    }
    if (after_flush) {
        (*after_flush)();
    }
}

void init()
{
    emscripten_set_main_loop(brunhild_diff_DOM, 0, 0);
    EM_ASM({
        document.body.id = "bh-0";
        document.body.innerHTML = "";
    });
}
}

// void View::on(std::string type, std::string selector, Handler handler)
// {
//     // Need to prepend root node ID to all selectors
//     std::ostringstream s;
//     if (const string id_str = id(); selector != "") {
//         std::string_view view = { selector };
//         size_t i;
//         while (1) {
//             i = view.find(',');
//             auto frag = view.substr(0, i);

//             // If this comma is inside a selector like :not(.foo,.bar), skip
//             the
//             // appropriate amount of closing brackets. Assumes correct CSS
//             // syntax.
//             const auto opening = std::count(frag.begin(), frag.end(), '(');
//             if (opening) {
//                 const auto closing = std::count(frag.begin(), frag.end(),
//                 ')'); if (closing != opening) {
//                     i = view.find(',', view.find(")", i, closing - opening));
//                     frag = view.substr(0, i);
//                 }
//             }

//             s << '#' << id_str << ' ' << frag;
//             if (i != std::string::npos) {
//                 view = view.substr(i + 1);
//                 s << ',';
//             } else {
//                 break;
//             }
//         }
//     } else {
//         // Select all children, if no selector
//         s << '#' << id_str << " *";
//     }

//     event_handlers.push_back(register_handler(type, handler, s.str()));
// }

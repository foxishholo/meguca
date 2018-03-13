#pragma once

#include "events.hh"
#include <emscripten/val.h>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace brunhild {

// Element attributes. std::nullopt values are omitted from rendered HTML.
typedef std::unordered_map<std::string, std::string> Attrs;

// Escape a user-submitted unsafe string to protect against XSS and malformed
// HTML
std::string escape(const std::string& s);

// Nodes are simple lightweight  logicless representations of HTML nodes. Place
// into Children and return from View.children().
struct Node {
    // Tag of the Element
    std::string tag;

    // Attributes and properties of the Element
    Attrs attrs;

    // Children of the element
    std::vector<Node> children;

    // Inner HTML of the Element. If set, children are ignored
    std::optional<std::string> inner_html;

    // Creates a Node with optional attributes and children
    Node(std::string tag, Attrs attrs = {}, std::vector<Node> children = {});

    // Creates a Node with HTML set as the inner contents.
    // escape: specifies, if the text should be escaped
    Node(std::string tag, Attrs attrs, std::string html, bool escape = false);

    // Creates a Node with html set as the inner contents.
    // escape: specifies, if the text should be escaped
    Node(std::string tag, std::string html, bool escape = false);

    Node() = default;
};

class View;

// Encapsulates either a list of child Views or Nodes or an HTML string
struct Children {
    // Encapsulate a vector of Views as Children
    Children(std::vector<std::shared_ptr<View>> children);

    // Encapsulate a vector of lightweight Nodes as Children
    Children(std::vector<Node> children);

    // Encapsulate an HTML string as Children
    // escape: specifies, if the text should be escape
    Children(std::string html, bool escape = false);

    Children() = default;

    // Type of contained children types
    enum class Type { views, nodes, html };

    Type type;
    std::vector<std::shared_ptr<View>> views;
    std::vector<Node> nodes;
    std::string html;
};

class View : std::enable_shared_from_this<View> {
public:
    // View root node and subtree can never mutate. This is guaranteed by the
    // library user.
    const bool is_const;

    // Unique ID
    const unsigned long id;

    // Tag of root Node
    const std::string tag;

    // tag: root node HTML tag
    // is_const: the caller guaranties that neither the root element or subtree
    // of this view will ever mutate
    View(std::string tag = "div", bool is_const = false);

    // Returns root node attributes. Must not contain "id" key.
    virtual Attrs attrs() { return {}; };

    // Returns children of View
    virtual Children children() { return {}; }

    // Mark View as needing a diff and patch. Not calling patch() on a modified
    // view or one of its parents (no matter how many levels up) will not yield
    // any DOM patches and can produce out of index reads.
    void patch();

    // Scroll the view's element into the browser's viewport
    void scroll_into_view();

protected:
    // Any custom initialization logic to run after rendering the element for
    // the first time or reattaching the element to a parent after removal.
    virtual void on_mount(){};
};
}

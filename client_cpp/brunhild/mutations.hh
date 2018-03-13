#pragma once

#include <string>

namespace brunhild {
// Append a node to a parent
void append(unsigned long id, std::string html);

// Prepend a node to a parent
void prepend(unsigned long id, std::string html);

// Insert a node before a sibling
void before(unsigned long id, std::string html);

// Insert a node after a sibling
void after(unsigned long id, std::string html);

// Set inner html of an element
void set_inner_html(unsigned long id, std::string html);

// Set outer html of an element
void set_outer_html(unsigned long id, std::string html);

// Remove an element
void remove(unsigned long id);

// Set an element attribute to a value
void set_attr(unsigned long id, std::string key, std::string val);

// Remove an element attribute
void remove_attr(unsigned long id, std::string key);

// Scroll and element into the viewport
void scroll_into_view(unsigned long id);
}

use std::collections::{HashMap, HashSet};
use std::cell::RefCell;
use stdweb::Value;

thread_local!{
	// All pending mutations quickly accessible by element ID
	static MUTATIONS: RefCell<HashMap<String, Mutations>>
		= RefCell::new(HashMap::new());

	// Stores mutation order, so we can somewhat make sure, new children are not
	// manipulated before insertion
	static ORDER: RefCell<Vec<String>> = RefCell::new(vec![]);

	static BEFORE_FLUSH: RefCell<Option<&'static Fn()>> = RefCell::new(None);
	static AFTER_FLUSH: RefCell<Option<&'static Fn()>> = RefCell::new(None);
}

// Pending mutations for an element
#[derive(Default)]
struct Mutations {
	remove_el: bool,
	scroll_into_view: bool,
	set_inner_html: Option<String>,
	set_outer_html: Option<String>,
	append: Vec<String>,
	prepend: Vec<String>,
	before: Vec<String>,
	after: Vec<String>,
	move_prepend: Vec<String>,
	move_after: Vec<String>,
	remove_attr: HashSet<String>,
	set_attr: HashMap<String, String>,
}

// Fetches a mutation set by element ID or creates a new one ond registers
// its execution order
fn with_mutations<F>(id: &str, func: F)
where
	F: Fn(&mut Mutations),
{
	MUTATIONS.with(|m| {
		ORDER.with(|o| {
			let mutations = &mut *m.borrow_mut();
			if !mutations.contains_key(id) {
				(*o.borrow_mut()).push(id.to_string());
				mutations.insert(id.to_string(), Mutations::default());
			}
			func(mutations.get_mut(id).unwrap());
		});
	});
}

macro_rules! push_functions {
	($( $x:ident ),*) => {
		$(
			pub fn $x(id: &str, html: &str) {
				with_mutations(id, |m| {
					m.$x.push(html.to_string())
				})
			}
		)*
	};
}

push_functions!(
	append,
	prepend,
	before,
	after,
	move_prepend,
	move_after
);

pub fn set_inner_html(id: &str, html: &str) {
	with_mutations(id, |m| {
		m.free_inner();
		m.set_inner_html = Some(html.to_string());
	})
}

pub fn set_outer_html(id: &str, html: &str) {
	with_mutations(id, |m| {
		m.free_outer();
		m.set_outer_html = Some(html.to_string());
	})
}

pub fn remove(id: &str) {
	with_mutations(id, |m| {
		m.free_outer();
		m.remove_el = true;
	})
}

pub fn set_attr(id: &str, key: &str, val: &str) {
	with_mutations(id, |m| {
		m.set_attr
			.insert(key.to_string(), val.to_string());
	})
}

pub fn remove_attr(id: &str, key: &str) {
	with_mutations(id, |m| {
		m.set_attr.remove(key);
		m.remove_attr.insert(key.to_string());
	})
}

pub fn scroll_into_view(id: &str) {
	with_mutations(id, |m| {
		m.scroll_into_view = true;
	})
}

impl Mutations {
	// Clear mutations of element inner content to free up memory and remove
	// conflicts
	pub fn free_inner(&mut self) {
		self.append.clear();
		self.prepend.clear();
		self.move_prepend.clear();
		self.move_after.clear();
		self.set_inner_html = None;
	}

	// Clear mutations of element inner and outer content to free up memory and
	// remove conflicts
	pub fn free_outer(&mut self) {
		self.free_inner();
		self.remove_attr.clear();
		self.set_attr.clear();
		self.set_outer_html = None;
	}

	// Execute buffered mutations
	pub fn exec(&self, id: &str) {
		// Assign element to global variable, so we don't have to look it up
		// each time
		let exists = match js! {
			window.__el = document.getElementById(@{id});
			return !!window.__el;
		} {
			Value::Bool(b) => b,
			_ => false,
		};
		if !exists {
			return;
		}

		// TODO: Do these loops in less JS calls, if possible

		for html in &self.before {
			js!{
				var el = window.__el;
				var cont = document.createElement("div");
				cont.innerHTML = @{html};
				el.parentNode.insertBefore(cont.firstChild, el);
			};
		}
		for html in &self.after {
			js!{
				var el = window.__el;
				var cont = document.createElement("div");
				cont.innerHTML = @{html};
				el.parentNode.insertBefore(cont.firstChild, el.nextSibling);
			};
		}

		if self.remove_el {
			js!{
				var el = window.__el;
				el.parentNode.removeChild(el);
			};
			// If the element is to be removed, nothing else needs to be done
			return;
		}

		if let &Some(ref html) = &self.set_outer_html {
			js!{ window.__el.outerHTML = @{html}; }
		}
		if let &Some(ref html) = &self.set_inner_html {
			js!{ window.__el.innerHTML = @{html}; }
		}

		for html in &self.append {
			js!{
				var el = window.__el;
				var cont = document.createElement("div");
				cont.innerHTML = @{html};
				el.appendChild(cont.firstChild);
			};
		}
		for html in &self.prepend {
			js!{
				var el = window.__el;
				var cont = document.createElement("div");
				cont.innerHTML = @{html};
				el.insertBefore(cont.firstChild, el.firstChild);
			};
		}
		for html in &self.move_prepend {
			js!{
				var el = window.__el;
				el.insertBefore(
					document.getElementById(@{html}), el.firstChild);
			};
		}
		for html in &self.move_after {
			js!{
				var el = window.__el;
				el.parentNode.insertBefore(
					document.getElementById(@{html}), el.nextSibling);
			};
		}

		for (k, v) in &self.set_attr {
			js!{ window.__el.setAttribute(@{k}, @{v}); };
		}
		for k in &self.remove_attr {
			js!{ window.__el.removeAttribute(@{k}); };
		}

		if self.scroll_into_view {
			js!{ window.__el.scrollIntoView(); }
		}
	}
}

// Flush all pending DOM mutations
pub extern "C" fn flush() {
	BEFORE_FLUSH.with(|f| {
		if let Some(f) = *f.borrow() {
			f();
		}
	});

	MUTATIONS.with(|m| {
		ORDER.with(|o| {
			let mutations = &mut *m.borrow_mut();
			let order = &mut *o.borrow_mut();
			if mutations.len() != 0 {
				for id in order.iter() {
					mutations.get(id).unwrap().exec(id);
				}
				order.clear();
				mutations.clear();
			}
		});
	});

	AFTER_FLUSH.with(|f| {
		if let Some(f) = *f.borrow() {
			f();
		}
	});
}

// Sets function to run before flushing mutations
pub fn run_before_flush(func: Option<&'static Fn()>) {
	BEFORE_FLUSH.with(|f| f.replace(func));
}

// Sets function to after before flushing mutations
pub fn run_after_flush(func: Option<&'static Fn()>) {
	AFTER_FLUSH.with(|f| f.replace(func));
}

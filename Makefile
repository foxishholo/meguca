export node_bins=$(PWD)/node_modules/.bin
export uglifyjs=$(node_bins)/uglifyjs
export gulp=$(node_bins)/gulp
export is_windows=false
binary=meguca
ifeq ($(GOPATH),)
	export PATH:=$(PATH):$(HOME)/go/bin
	export GOPATH=$(HOME)/go:$(PWD)/go
else
	export PATH:=$(PATH):$(GOPATH)/bin
	export GOPATH:=$(GOPATH):$(PWD)/go
endif

# Differentiate between Unix and mingw builds
ifeq ($(OS), Windows_NT)
	export PKG_CONFIG_PATH:=$(PKG_CONFIG_PATH):/mingw64/lib/pkgconfig/
	export PKG_CONFIG_LIBDIR=/mingw64/lib/pkgconfig/
	export PATH:=$(PATH):/mingw64/bin/
	export is_windows=true
	binary=meguca.exe
endif

.PHONY: server client imager test

all: server client

client: client_vendor
	$(gulp)

client_deps:
	npm install --progress false --depth 0

wasm:
	mkdir -p www/wasm
	$(MAKE) -C client_cpp
	rm -f www/wasm/main.*
	cp client_cpp/*.wasm client_cpp/*.js www/wasm
ifeq ($(DEBUG),1)
	cp client_cpp/*.wast client_cpp/*.wasm.map www/wasm
endif

wasm_clean:
	# $(MAKE) -C client_cpp clean
	rm -f www/wasm/*.js www/wasm/*.wasm www/wasm/*.map www/wasm/*.wast

watch:
	$(gulp) -w

client_vendor: client_deps
	mkdir -p www/js/vendor
	cp node_modules/dom4/build/dom4.js node_modules/core-js/client/core.min.js node_modules/core-js/client/core.min.js.map www/js/vendor
	$(uglifyjs) node_modules/whatwg-fetch/fetch.js -o www/js/vendor/fetch.js
	$(uglifyjs) node_modules/almond/almond.js -o www/js/vendor/almond.js

css:
	$(gulp) css

server: generate server_deps
	go build -v -o $(binary) meguca
ifeq ($(is_windows), true)
	cp /mingw64/bin/*.dll ./
endif

generate: generate_clean
	go get -v github.com/valyala/quicktemplate/qtc github.com/jteeuwen/go-bindata/... github.com/mailru/easyjson/... github.com/bakape/thumbnailer github.com/gorilla/websocket
	go generate meguca/...
	cd go/src/meguca/codec/internal; swig -c++ -go -cgo -intgosize 64 codec.i

generate_clean:
	rm -f go/src/meguca/db/bin_data.go go/src/meguca/lang/bin_data.go go/src/meguca/assets/bin_data.go
	rm -f go/src/meguca/common/*_easyjson.go
	rm -f go/src/meguca/config/*_easyjson.go
	rm -f go/src/meguca/websockets/feeds/*_easyjson.go
	rm -f go/src/meguca/templates/*.qtpl.go
	rm -f go/src/meguca/codec/internal/codec.go go/src/meguca/codec/internal/*_wrap.cxx

server_deps:
	go list -f '{{.Deps}}' meguca | tr -d '[]' | xargs go get -v

update_deps:
	go get -u -v github.com/valyala/quicktemplate/qtc github.com/jteeuwen/go-bindata/... github.com/mailru/easyjson/...
	go list -f '{{.Deps}}' meguca | tr -d '[]' | xargs go list -e -f '{{if not .Standard}}{{.ImportPath}}{{end}}' | grep -v 'meguca' | xargs go get -u -v

client_clean:
	rm -rf www/js www/css/*.css www/css/maps node_modules

clean: client_clean wasm_clean generate_clean
	rm -rf .build .ffmpeg .package go/pkg target meguca-*.zip meguca-*.tar.xz meguca meguca.exe
ifeq ($(is_windows), true)
	rm -rf /.meguca_build *.dll
endif

dist_clean: clean
	rm -rf images error.log db.db

test:
	go test --race -p 1 meguca/...

test_no_race:
	go test -p 1 meguca/...

check: test

upgrade_v4: generate
	go get -v github.com/dancannon/gorethink
	$(MAKE) -C scripts/migration/3to4 upgrade

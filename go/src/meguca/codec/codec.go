package codec

// #include <stdlib.h>
import "C"
import (
	"fmt"
	codec "meguca/codec/internal"
	"meguca/common"
	"unsafe"
)

// Encode a compatible type to a websocket message with a type prefix byte
func Encode(typ common.MessageType, data interface{}) ([]byte, error) {
	var (
		buf  uintptr
		size int64
	)
	defer func() {
		if buf != 0 {
			C.free(unsafe.Pointer(buf))
		}
	}()

	switch data.(type) {
	case string:
		buf = codec.Encode_string(&size, data.(string))
	case byte:
		buf = codec.Encode_byte(&size, data.(byte))
	default:
		return nil, fmt.Errorf("no encoder for type %T", data)
	}

	b := make([]byte, 1, size+1)
	b[0] = byte(typ)
	return append(b, C.GoBytes(unsafe.Pointer(buf), C.int(size))...), nil
}

package common

// Websocket protocol version
const ProtocolVersion = 1

// MessageType is the identifier code for websocket message types
type MessageType uint8

// 1 - 29 modify post model state
const (
	MessageInvalid MessageType = iota
	_
	MessageInsertPost
	MessageAppend
	MessageBackspace
	MessageSplice
	MessageClosePost
	_
	MessageInsertImage
	MessageSpoiler
	MessageDeletePost
	MessageBanned
	MessageDeleteImage
)

// >= 30 are miscellaneous and do not write to post models
const (
	MessageSynchronise MessageType = 30 + iota
	MessageReclaim

	// Send new post ID to client
	MessagePostID

	// Concatenation of multiple websocket messages to reduce transport overhead
	MessageConcat

	// Message from the client meant to invoke no operation. Mostly used as a
	// one way ping, because the JS Websocket API does not provide access to
	// pinging.
	MessageNOOP

	// Transmit current synced IP count to client
	MessageSyncCount

	// Send current server Unix time to client
	MessageServerTime

	// Redirect the client to a specific board
	MessageRedirect

	// Send a notification to a client
	MessageNotification

	// Notify the client, he needs a captcha solved
	MessageCaptcha

	// Passes MeguTV playlist data
	MessageMeguTV

	// Used by the client to send it's protocol version and by the server to
	// send server and board configurations
	MessageConfigs
)

// Forwarded functions from "meguca/websockets/feeds" to avoid circular imports
var (
	// GetByIPAndBoard retrieves all Clients that match the passed IP on a board
	GetByIPAndBoard func(ip, board string) []Client

	// Return connected clients with matching ip
	GetClientsByIp func(ip string) []Client

	// SendTo sends a message to a feed, if it exists
	SendTo func(id uint64, msg []byte)

	// ClosePost closes a post in a feed, if it exists
	ClosePost func(
		id, op uint64,
		links []Link,
		commands []Command,
		msg []byte,
	)

	// Propagate a message about a post being banned
	BanPost func(id, op uint64) error

	// Propagate a message about a post being deleted
	DeletePost func(id, op uint64) error

	// Propagate a message about an image being deleted from a post
	DeleteImage func(id, op uint64) error

	// Propagate a message about an image being spoilered
	SpoilerImage func(id, op uint64) error
)

// Client exposes some globally accessible websocket client functionality
// without causing circular imports
type Client interface {
	Send([]byte)
	Redirect(board string)
	IP() string
	Close(error)
}

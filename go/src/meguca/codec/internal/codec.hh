#pragma once

#include <algorithm>
#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <sstream>
#include <string>

// // MessageType is the identifier code for websocket message types
// enum class Type {
//     invalid,
//     insert_post,
//     append,
//     backspace,
//     splice,
//     close_post,
//     insert_image,
//     spoiler,
//     delete_post,
//     banned,
//     delete_image,

//     synchronise = 30,
//     reclaim,
//     // Send new post ID to client
//     post_id,
//     // Concatenation of multiple websocket messages to reduce transport
//     overhead
//     concat,
//     // Message from the client meant to invoke no operation. Mostly used as a
//     // one way ping, because the JS Websocket API does not provide access to
//     // pinging.
//     noop,
//     // Transmit current synced IP count to client
//     sync_count,
//     // Send current server Unix time to client
//     server_time,
//     // Redirect the client to a specific board
//     redirect,
//     // Send a notification to a client
//     notification,
//     // Notify the client, he needs a captcha solved
//     captcha,
//     // Passes MeguTV playlist data
//     megu_tv,
// };

struct PostLink {
    unsigned long id, op;
    std::string board;

    template <class Archive> void serialize(Archive& archive)
    {
        archive(id, op, board);
    }
};

struct Command {
    int typ;
    bool flip;
    unsigned long pyu, syncwatch[5];
    char* eightball;
    std::vector<unsigned short> dice;
    char roulette[2];

    ~Command()
    {
        if (eightball) {
            free(eightball);
        }
    }
};

// Encodes message to binary
template <class T> void* encode(size_t* size, T msg)
{
    std::ostringstream s;
    cereal::BinaryOutputArchive ar(s);
    ar(msg);

    auto str = s.str();
    *size = str.size();
    return memcpy(malloc(*size), str.data(), *size);
}

// Wrapper to encode char*, which Cereal does not support natively
void* encode_string(size_t* size, char* msg)
{
    return encode(size, std::string(msg));
}


%module codec
%include "std_vector.i"
%{
#include "codec.hh"
%}
%include "codec.hh"
%template(encode_byte) encode<char>;
%template(vector_uint16) std::vector<unsigned short>;
%template(vector_command) std::vector<Command>;
%template(vector_post_link) std::vector<PostLink>;

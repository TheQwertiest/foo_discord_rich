#pragma once

#include <string>

// `operator>>` and `operator<<` overloads for various fb2k SDK streams

FB2K_STREAM_READER_OVERLOAD( std::string )
{
    t_uint32 len;
    stream >> len;

    value.resize( len + 1 );
    stream.m_stream.read_object( value.data(), len, stream.m_abort );
    value.resize( strlen( value.c_str() ) );

    return stream;
}

FB2K_STREAM_WRITER_OVERLOAD( std::string )
{
    stream << pfc::downcast_guarded<t_uint32>( value.length() );
    stream.write_raw( value.c_str(), value.length() );
    return stream;
}

namespace drp::pfc_x
{

std::u8string ReadString( stream_reader& stream, abort_callback& abort );
std::u8string ReadRawString( stream_reader& stream, abort_callback& abort );
void WriteString( stream_writer& stream, const std::u8string& val, abort_callback& abort );
void WriteStringRaw( stream_writer& stream, const std::u8string& val, abort_callback& abort );

} // namespace drp::pfc_x

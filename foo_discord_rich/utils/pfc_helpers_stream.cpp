#include <stdafx.h>
#include "pfc_helpers_stream.h"

namespace drp::pfc_x
{

std::u8string ReadString( stream_reader& stream, abort_callback& abort )
{ // ripped from `stream_reader::read_string`
    t_uint32 length;
    stream.read_lendian_t( length, abort );

    std::u8string value;
    value.resize( length );
    stream.read_object( value.data(), length, abort );
    value.resize( strlen( value.c_str() ) );

    return value;
}

std::u8string ReadRawString( stream_reader& stream, abort_callback& abort )
{ // ripped from `stream_reader::read_string_raw`
    constexpr size_t maxBufferSize = 256;
    char buffer[maxBufferSize];
    std::u8string value;

    bool hasMoreData = true;
    do
    {
        const size_t dataRead = stream.read( buffer, sizeof( buffer ), abort );
        hasMoreData = ( dataRead == maxBufferSize );
        if ( dataRead )
        {
            value.append( buffer, dataRead );
        }
    } while ( hasMoreData );

    return value;
}

void WriteString( stream_writer& stream, const std::u8string& val, abort_callback& abort )
{
    stream.write_string( val.c_str(), val.length(), abort );
}

void WriteStringRaw( stream_writer& stream, const std::u8string& val, abort_callback& abort )
{
    stream.write_object( val.c_str(), val.length(), abort );
}

} // namespace drp::pfc_x

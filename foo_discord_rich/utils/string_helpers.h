#pragma once

#include <sstream>

namespace utils::string
{

class Formatter
{
public:
    Formatter() = default;
    ~Formatter() = default;
    Formatter( const Formatter& ) = delete;
    Formatter& operator=( Formatter& )  = delete;

    template <typename Type>
    Formatter& operator<<( const Type& value )
    {
        stream_ << value;
        return *this;
    }

    std::string str() const
    {
        return stream_.str();
    }
    operator std::string() const
    {
        return stream_.str();
    }

private:
    std::stringstream stream_;
};

std::string Trim( const std::string& str );
std::wstring Trim( const std::wstring& str );
pfc::string8_fast Trim( const pfc::string8_fast& str );

}

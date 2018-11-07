#pragma once

namespace utils
{

template <typename T, typename InnerT>
class CfgWrap
{
public:
    template <typename ArgT, class = typename std::enable_if_t<std::is_convertible_v<ArgT, InnerT>>>
    CfgWrap( const GUID& guid, const ArgT& defaultValue )
        : conf_( guid, defaultValue )
        , curValue_( conf_ )
        , cachedConfValue_( conf_ )
        , defValue_( defaultValue )
    {
    }

    CfgWrap( const CfgWrap& ) = delete;

    void Reread()
    {
        curValue_ = conf_;
        cachedConfValue_ = conf_;
        hasChanged_ = false;
    }

    template <typename ArgT, class = typename std::enable_if_t<std::is_convertible_v<ArgT, InnerT>>>
    CfgWrap& operator=( const ArgT& value )
    {
        SetValue( value );
        return *this;
    }
    operator const InnerT&() const
    {
        return GetSavedValue();
    }

    bool HasChanged() const
    {
        return hasChanged_;
    }
    const InnerT& GetSavedValue() const
    {
        return cachedConfValue_;
    }
    const InnerT& GetCurrentValue() const
    {
        return curValue_;
    }
    template <typename ArgT, class = typename std::enable_if_t<std::is_convertible_v<ArgT, InnerT>>>
    void SetValue( const ArgT& value )
    {
        if ( curValue_ != value )
        {
            curValue_ = value;
            hasChanged_ = true;
        }
    }
    void Apply()
    {
        if ( hasChanged_ )
        {
            if constexpr ( std::is_same_v<T, cfg_string> )
            {
                conf_ = curValue_.c_str();
            }
            else
            {
                conf_ = curValue_;
            }
            cachedConfValue_ = conf_;
            hasChanged_ = false;
        }
    }
    void Reset()
    {
        curValue_ = defValue_;
        hasChanged_ = ( defValue_ != conf_ );
    }

private:
    bool hasChanged_ = false;
    T conf_;
    InnerT cachedConfValue_;
    InnerT curValue_;
    const InnerT defValue_;
};

} // namespace utils

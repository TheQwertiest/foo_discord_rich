#pragma once

namespace utils
{

/// @brief cfg_var wrapper for preferences page.
/// @details Provides `apply`, `revert` and `reset to default` functionality.
class ICfgWrap
{
public:
    ICfgWrap() = default;
    virtual ~ICfgWrap() = default;

    virtual void Reread() = 0;
    virtual bool HasChanged() const = 0;
    virtual void Apply() = 0;
    virtual void Revert() = 0;
    virtual void ResetToDefault() = 0;
};

/// @brief cfg_var wrapper for preferences page.
/// @details Provides `apply`, `revert` and `reset to default` functionality.
template <typename T, typename InnerT>
class CfgWrap
    : public ICfgWrap
{
public:
    template <typename ArgT, class = typename std::enable_if_t<std::is_convertible_v<ArgT, InnerT>>>
    CfgWrap( const GUID& guid, const ArgT& defaultValue )
        : conf_( guid, defaultValue )
        , curValue_( conf_ )
        , cachedConfValue_( conf_ )
        , defValue_( defaultValue )
    {
        static_assert( std::is_base_of_v<cfg_var, T> );
    }
    ~CfgWrap() override = default;

    CfgWrap( const CfgWrap& ) = delete;

    void Reread() override
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

    bool HasChanged() const override
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
        hasChanged_ = ( cachedConfValue_ != value );
        if ( curValue_ != value )
        {
            curValue_ = value;
        }
    }
    void Apply() override
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
    void Revert() override
    {
        if ( hasChanged_ )
        {
            curValue_ = cachedConfValue_;
            hasChanged_ = false;
        }
    }
    void ResetToDefault() override
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

using CfgWrapString8 = CfgWrap<cfg_string, pfc::string8_fast>;
using CfgWrapBool = CfgWrap<cfg_bool, bool>;
using CfgWrapUint8 = CfgWrap<cfg_int_t<uint8_t>, uint8_t>;

} // namespace utils

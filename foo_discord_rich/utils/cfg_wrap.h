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
template <typename CfgT, typename ValueT>
class CfgWrap
    : public ICfgWrap
{
public:
    template <typename ArgT, class = typename std::enable_if_t<std::is_convertible_v<ArgT, ValueT>>>
    CfgWrap( const GUID& guid, const ArgT& defaultValue )
        : conf_( guid, defaultValue )
        , curValue_( conf_ )
        , cachedConfValue_( conf_ )
        , defValue_( defaultValue )
    {
        static_assert( std::is_base_of_v<cfg_var, CfgT> );
    }
    ~CfgWrap() override = default;

    CfgWrap( const CfgWrap& ) = delete;

    template <typename ArgT, class = typename std::enable_if_t<std::is_convertible_v<ArgT, ValueT>>>
    CfgWrap& operator=( const ArgT& value )
    {
        SetValue( value );
        return *this;
    }
    operator const ValueT&() const
    {
        return GetSavedValue();
    }

    void Reread() override
    {
        curValue_ = conf_;
        cachedConfValue_ = conf_;
        hasChanged_ = false;
    }

    bool HasChanged() const override
    {
        return hasChanged_;
    }
    const ValueT& GetSavedValue() const
    {
        return cachedConfValue_;
    }
    const ValueT& GetCurrentValue() const
    {
        return curValue_;
    }
    template <typename ArgT, class = typename std::enable_if_t<std::is_convertible_v<ArgT, ValueT>>>
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
            conf_ = curValue_;
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
    CfgT conf_;
    ValueT cachedConfValue_;
    ValueT curValue_;
    const ValueT defValue_;
};

template <class CfgT, class ValueT>
bool operator!=( const CfgWrap<CfgT, ValueT>& left, const ValueT& right )
{
    return left.GetCurrentValue() != right;
}

template <class CfgT, class ValueT>
bool operator!=( const ValueT& left, const CfgWrap<CfgT, ValueT>& right )
{
    return right != left;
}

template <class CfgT, class ValueT>
bool operator==( const CfgWrap<CfgT, ValueT>& left, const ValueT& right )
{
    return left.GetCurrentValue() == right;
}

template <class CfgT, class ValueT>
bool operator==( const ValueT& left, const CfgWrap<CfgT, ValueT>& right )
{
    return right == left;
}

using CfgWrapString8 = CfgWrap<drp::pfc_x::cfg_std_string, std::u8string>;
using CfgWrapBool = CfgWrap<cfg_bool, bool>;
using CfgWrapUint8 = CfgWrap<cfg_int_t<uint8_t>, uint8_t>;

} // namespace utils

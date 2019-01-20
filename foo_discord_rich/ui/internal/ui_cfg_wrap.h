#pragma once

#include <utils/cfg_wrap.h>
#include <resource.h>

#include <initializer_list>
#include <vector>

namespace drp::ui
{

/// @brief utils::ICfgWrap wrapper for communication with UI elements.
class IUiCfgWrap
{
public:
    IUiCfgWrap() = default;
    virtual ~IUiCfgWrap() = default;

    virtual bool IsMatchingId( int controlId ) const = 0;

    virtual void SetHwnd( HWND hWnd ) = 0;
    virtual void ReadFromUi() = 0;
    virtual void WriteToUi() = 0;

    virtual utils::ICfgWrap& GetCfg() = 0;
    virtual const utils::ICfgWrap& GetCfg() const = 0;
};

/// @brief utils::ICfgWrap wrapper for communication with UI elements.
/// @details CfgWrapString8 - uGetDlgItemText//uSetDlgItemText
///          CfgWrapBool - uButton_GetCheck//uButton_SerCheck
template <typename T>
class UiCfgWrap
    : public IUiCfgWrap
{
public:
    UiCfgWrap( T& cfgWrap, int controlId )
        : cfgWrap_( cfgWrap )
        , controlId_( controlId )
    {
        static_assert( std::is_base_of_v<utils::ICfgWrap, T> );
    }
    ~UiCfgWrap() override = default;

    bool IsMatchingId( int controlId ) const override
    {
        return ( controlId == controlId_ );
    }

    void SetHwnd( HWND hWnd ) override
    {
        hWnd_ = hWnd;
    }

    void ReadFromUi() override
    {
        if ( !hWnd_ )
        {
            return;
        }

        if constexpr ( std::is_same_v<T, utils::CfgWrapBool> )
        {
            cfgWrap_ = uButton_GetCheck( hWnd_, controlId_ );
        }
        else if constexpr ( std::is_same_v<T, utils::CfgWrapString8> )
        {
            pfc::string8_fast tmp;
            if ( uGetDlgItemText( hWnd_, controlId_, tmp ) )
            {
                cfgWrap_ = tmp;
            }
        }
        else
        {
            static_assert( 0, "Unsupported type" );
        }
    }
    void WriteToUi() override
    {
        if ( !hWnd_ )
        {
            return;
        }

        if constexpr ( std::is_same_v<T, utils::CfgWrapBool> )
        {
            uButton_SetCheck( hWnd_, controlId_, cfgWrap_.GetCurrentValue() );
        }
        else if constexpr ( std::is_same_v<T, utils::CfgWrapString8> )
        {
            uSetDlgItemText( hWnd_, controlId_, cfgWrap_.GetCurrentValue() );
        }
        else
        {
            static_assert( 0, "Unsupported type" );
        }
    }

    utils::ICfgWrap& GetCfg() override
    {
        return cfgWrap_;
    }
    const utils::ICfgWrap& GetCfg() const override
    {
        return cfgWrap_;
    }

private:
    T& cfgWrap_;
    HWND hWnd_ = nullptr;
    const int controlId_;
};

/// @brief utils::ICfgWrap wrapper for communication with UI elements.
/// @details Maps to a range of radio buttons, communicates via uButton_GetCheck//uButton_SetCheck.
class UiCfgWrapRange
    : public IUiCfgWrap
{
public:
    UiCfgWrapRange( utils::CfgWrapUint8& cfgWrap, std::initializer_list<int> controlIdList )
        : cfgWrap_( cfgWrap )
        , controlIdList_( controlIdList )
    {
    }
    ~UiCfgWrapRange() override = default;

    bool IsMatchingId( int controlId ) const override
    {
        return ( controlIdList_.cend() != std::find( controlIdList_.cbegin(), controlIdList_.cend(), controlId ) );
    }

    void SetHwnd( HWND hWnd ) override
    {
        hWnd_ = hWnd;
    }

    void ReadFromUi() override
    {
        if ( !hWnd_ )
        {
            return;
        }

        for ( size_t i = 0; i < controlIdList_.size(); ++i )
        {
            if ( uButton_GetCheck( hWnd_, controlIdList_[i] ) )
            {
                cfgWrap_ = static_cast<uint8_t>( i );
            }
        }
    }
    void WriteToUi() override
    {
        if ( !hWnd_ )
        {
            return;
        }

        for ( size_t i = 0; i < controlIdList_.size(); ++i )
        {
            uButton_SetCheck( hWnd_, controlIdList_[i], cfgWrap_.GetCurrentValue() == static_cast<uint8_t>( i ) );
        }
    }

    utils::ICfgWrap& GetCfg() override
    {
        return cfgWrap_;
    }
    const utils::ICfgWrap& GetCfg() const override
    {
        return cfgWrap_;
    }

private:
    utils::CfgWrapUint8& cfgWrap_;
    HWND hWnd_ = nullptr;
    const std::vector<int> controlIdList_;
};

template <typename T>
std::unique_ptr<IUiCfgWrap> CreateUiCfgWrap( T& cfgWrap, int controlId )
{
    static_assert( std::is_base_of_v<utils::ICfgWrap, T> );
    return std::make_unique<UiCfgWrap<T>>( cfgWrap, controlId );
}

template <typename T>
std::unique_ptr<IUiCfgWrap> CreateUiCfgWrapRange( T& cfgWrap, std::initializer_list<int> controlIdList )
{
    static_assert( std::is_base_of_v<utils::ICfgWrap, T> );
    return std::make_unique<UiCfgWrapRange>( cfgWrap, controlIdList );
}

} // namespace drp::ui

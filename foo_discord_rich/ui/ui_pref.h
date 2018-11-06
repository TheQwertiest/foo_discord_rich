#pragma once

#include <resource.h>
#include <component_defines.h>

namespace drp::ui
{

class CDialogPref
    : public CDialogImpl<CDialogPref>
    , public CWinDataExchange<CDialogPref>
    , public preferences_page_instance
{
public:
    enum
    {
        IDD = IDD_DIALOG_PREFERENCE
    };

    BEGIN_MSG_MAP( CDialogPref )
    MSG_WM_INITDIALOG( OnInitDialog )
    COMMAND_HANDLER_EX( IDC_TEXTBOX_STATE, EN_CHANGE, OnEditChange )
    COMMAND_HANDLER_EX( IDC_TEXTBOX_DETAILS, EN_CHANGE, OnEditChange )
    COMMAND_HANDLER_EX( IDC_TEXTBOX_PARTYID, EN_CHANGE, OnEditChange )
    COMMAND_HANDLER_EX( IDC_RADIO_IMG_LIGHT, BN_CLICKED, OnEditChange )
    COMMAND_HANDLER_EX( IDC_RADIO_IMG_DARK, BN_CLICKED, OnEditChange )
    COMMAND_HANDLER_EX( IDC_RADIO_IMG_DISABLED, BN_CLICKED, OnEditChange )
    COMMAND_HANDLER_EX( IDC_RADIO_TIME_ELAPSED, BN_CLICKED, OnEditChange )
    COMMAND_HANDLER_EX( IDC_RADIO_TIME_REMAINING, BN_CLICKED, OnEditChange )
    COMMAND_HANDLER_EX( IDC_RADIO_TIME_DISABLED, BN_CLICKED, OnEditChange )
    END_MSG_MAP()

public:
    CDialogPref( preferences_page_callback::ptr callback );

    static void InitConfiguration();

    // preferences_page_instance
    HWND get_wnd() override;
    t_uint32 get_state() override;
    void apply() override;
    void reset() override;

public:
    enum class ImageSetting : uint8_t
    {
        Light,
        Dark,
        Disabled
    };
    enum class TimeSetting : uint8_t
    {
        Elapsed,
        Remaining,
        Disabled
    };

    static bool IsEnabled();
    static ImageSetting GetImageSettings();
    static TimeSetting GetTimeSetting();
    static const pfc::string8_fast& GetStateQuery();
    static const pfc::string8_fast& GetDetailsQuery();
    static const pfc::string8_fast& GetPartyIdQuery();

private:
    BOOL OnInitDialog( HWND hwndFocus, LPARAM lParam );
    void OnEditChange( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnChanged();
    void UpdateUiFromCfg();

private:
    preferences_page_callback::ptr m_callback;

    template <typename T, typename InnerT>
    class CfgWrap
    {
    public:
        template <typename ArgT, class = typename std::enable_if_t<std::is_convertible_v<ArgT, InnerT>>>
        CfgWrap( const GUID& guid, const ArgT& defaultValue )
            : conf_( guid, defaultValue )
            , curValue_( conf_ )
            , defValue_( defaultValue )
        {
        }

        CfgWrap( const CfgWrap& ) = delete;

        void Reread()
        {
            curValue_ = conf_;
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
            return GetValue();
        }

        bool HasChanged() const
        {
            return hasChanged_;
        }
        const InnerT& GetValue() const
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
        InnerT curValue_;
        const InnerT defValue_;
    };

    static CfgWrap<cfg_bool, bool> isEnabled_;
    static CfgWrap<cfg_int_t<uint8_t>, uint8_t> imageSettings_;
    static CfgWrap<cfg_int_t<uint8_t>, uint8_t> timeSettings_;
    static CfgWrap<cfg_string, pfc::string8_fast> stateQuery_;
    static CfgWrap<cfg_string, pfc::string8_fast> detailsQuery_;
    static CfgWrap<cfg_string, pfc::string8_fast> partyIdQuery_;
};

} // namespace drp::ui

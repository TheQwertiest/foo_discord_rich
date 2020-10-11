#pragma once

namespace drp::ui
{

class ITab
{
public:
    virtual ~ITab() = default;

    virtual HWND CreateTab( HWND hParent ) = 0;
    virtual CDialogImplBase& Dialog() = 0;
    virtual const wchar_t* Name() const = 0;

    // preferences_page_instance

    //! @returns a combination of preferences_state constants.
    virtual uint32_t get_state() = 0;
    //! @returns the window handle.
    //! Applies preferences changes.
    virtual void apply() = 0;
    //! Resets this page's content to the default values. Does not apply any changes - lets user preview the changes before hitting "apply".
    virtual void reset() = 0;
};

} // namespace drp::ui
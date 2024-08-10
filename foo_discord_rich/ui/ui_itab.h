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

    virtual void OnUiChangeRequest( int nID, bool enable ) = 0;

    virtual uint32_t GetState() = 0;
    virtual void Apply() = 0;
    virtual void Reset() = 0;
};

} // namespace drp::ui

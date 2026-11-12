#include "Win32DragDrop.h"

#include <ole2.h>
#include <shlobj.h>

#include <cstring>
#include <string>

#include "SPDragDrop.h"

namespace space
{
    namespace win32
    {
        namespace
        {
            std::wstring widen(const std::string& text)
            {
                if (text.empty())
                    return {};
                const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
                std::wstring wide(static_cast<size_t>(length), L'\0');
                MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), length);
                return wide;
            }

            // Reference counting shared by the three COM objects below.
            template <typename Interface>
            class ComObject : public Interface
            {
            public:
                virtual ~ComObject() = default;

                HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **object) override
                {
                    if (!object)
                        return E_POINTER;
                    if (iid == IID_IUnknown || iid == __uuidof(Interface))
                    {
                        *object = static_cast<Interface *>(this);
                        AddRef();
                        return S_OK;
                    }
                    *object = nullptr;
                    return E_NOINTERFACE;
                }

                ULONG STDMETHODCALLTYPE AddRef() override { return static_cast<ULONG>(InterlockedIncrement(&references)); }

                ULONG STDMETHODCALLTYPE Release() override
                {
                    const LONG remaining = InterlockedDecrement(&references);
                    if (remaining == 0)
                        delete this;
                    return static_cast<ULONG>(remaining);
                }

            private:
                LONG references = 1;
            };

            // The dragged URIs as Unicode text, one per line.
            class TextDataObject final : public ComObject<IDataObject>
            {
            public:
                explicit TextDataObject(std::wstring text) : text(std::move(text)) {}

                HRESULT STDMETHODCALLTYPE GetData(FORMATETC *format, STGMEDIUM *medium) override
                {
                    if (!format || !medium)
                        return E_INVALIDARG;
                    const HRESULT supported = QueryGetData(format);
                    if (FAILED(supported))
                        return supported;
                    const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
                    const HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
                    if (!memory)
                        return E_OUTOFMEMORY;
                    std::memcpy(GlobalLock(memory), text.c_str(), bytes);
                    GlobalUnlock(memory);
                    medium->tymed = TYMED_HGLOBAL;
                    medium->hGlobal = memory;
                    medium->pUnkForRelease = nullptr;
                    return S_OK;
                }

                HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC *format) override
                {
                    if (!format)
                        return E_INVALIDARG;
                    if (format->cfFormat != CF_UNICODETEXT || format->dwAspect != DVASPECT_CONTENT)
                        return DV_E_FORMATETC;
                    return (format->tymed & TYMED_HGLOBAL) ? S_OK : DV_E_TYMED;
                }

                HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD direction, IEnumFORMATETC **enumerator) override
                {
                    if (direction != DATADIR_GET)
                        return E_NOTIMPL;
                    FORMATETC format{CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
                    return SHCreateStdEnumFmtEtc(1, &format, enumerator);
                }

                HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC *, STGMEDIUM *) override { return E_NOTIMPL; }
                HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC *, FORMATETC *output) override
                {
                    if (output)
                        output->ptd = nullptr;
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE SetData(FORMATETC *, STGMEDIUM *, BOOL) override { return E_NOTIMPL; }
                HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC *, DWORD, IAdviseSink *, DWORD *) override { return OLE_E_ADVISENOTSUPPORTED; }
                HRESULT STDMETHODCALLTYPE DUnadvise(DWORD) override { return OLE_E_ADVISENOTSUPPORTED; }
                HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA **) override { return OLE_E_ADVISENOTSUPPORTED; }

            private:
                std::wstring text;
            };

            class DropSource final : public ComObject<IDropSource>
            {
            public:
                HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL escapePressed, DWORD keyState) override
                {
                    if (escapePressed)
                        return DRAGDROP_S_CANCEL;
                    return (keyState & MK_LBUTTON) ? S_OK : DRAGDROP_S_DROP;
                }

                HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD) override { return DRAGDROP_S_USEDEFAULTCURSORS; }
            };

            // Where the drag last was over the window, so a resting pointer can be reported again.
            struct Hover
            {
                bool inside = false;
                int x = 0;
                int y = 0;
            };
            Hover hover;

            // Takes only the drags this process started: what they carry is
            // read from SPDragDrop, not from the data object.
            class DropTarget final : public ComObject<IDropTarget>
            {
            public:
                explicit DropTarget(HWND window) : window(window) {}

                HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *, DWORD keyState, POINTL point, DWORD *effect) override
                {
                    return DragOver(keyState, point, effect);
                }

                HRESULT STDMETHODCALLTYPE DragOver(DWORD, POINTL point, DWORD *effect) override
                {
                    if (!effect)
                        return E_INVALIDARG;
                    const POINT client = toClient(point);
                    hover = {SPDragDrop::active(), client.x, client.y};
                    *effect = SPDragDrop::over(client.x, client.y) ? DROPEFFECT_COPY : DROPEFFECT_NONE;
                    return S_OK;
                }

                HRESULT STDMETHODCALLTYPE DragLeave() override
                {
                    hover.inside = false;
                    SPDragDrop::leave();
                    return S_OK;
                }

                HRESULT STDMETHODCALLTYPE Drop(IDataObject *, DWORD, POINTL point, DWORD *effect) override
                {
                    if (!effect)
                        return E_INVALIDARG;
                    const POINT client = toClient(point);
                    hover.inside = false;
                    *effect = SPDragDrop::drop(client.x, client.y) ? DROPEFFECT_COPY : DROPEFFECT_NONE;
                    return S_OK;
                }

            private:
                POINT toClient(POINTL point) const
                {
                    POINT client{point.x, point.y};
                    ScreenToClient(window, &client);
                    return client;
                }

                HWND window;
            };

            // OLE need not report a pointer that is at rest. Targets act on a
            // hover (the sidebar opens a playlist), so repeat the last report.
            void CALLBACK hoverTimerProc(HWND, UINT, UINT_PTR, DWORD)
            {
                if (hover.inside && SPDragDrop::active())
                    SPDragDrop::over(hover.x, hover.y);
            }
        }

        void registerDropTarget(HWND window)
        {
            // S_FALSE when COM is already initialized on this thread, which still enables OLE.
            if (FAILED(OleInitialize(nullptr)))
                return;
            auto *target = new DropTarget(window);
            RegisterDragDrop(window, target);
            target->Release(); // the registration holds it now
        }

        void revokeDropTarget(HWND window)
        {
            RevokeDragDrop(window);
        }

        bool doDragDrop(const SPDragData& data)
        {
            std::wstring text;
            for (const auto& uri : data.uris)
            {
                if (!text.empty())
                    text += L"\r\n";
                text += widen(uri);
            }

            auto *object = new TextDataObject(std::move(text));
            auto *source = new DropSource();
            hover = {};
            const UINT_PTR timer = SetTimer(nullptr, 0, 100, hoverTimerProc);
            DWORD effect = DROPEFFECT_NONE;
            const HRESULT result = DoDragDrop(object, source, DROPEFFECT_COPY, &effect);
            if (timer)
                KillTimer(nullptr, timer);
            hover = {};
            source->Release();
            object->Release();
            return result == DRAGDROP_S_DROP && effect != DROPEFFECT_NONE;
        }
    }
}

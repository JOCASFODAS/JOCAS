// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <ksmedia.h>
#include <functiondiscoverykeys_devpkey.h>
#include <QString>
#include <stdexcept>

namespace Wasapi {
inline void check(HRESULT hr, const char *operation) {
    if (SUCCEEDED(hr)) return;
    QString detail;
    if (hr == E_ACCESSDENIED) detail = "Permita o acesso ao microfone para aplicativos da área de trabalho nas configurações de privacidade do Windows.";
    else if (hr == AUDCLNT_E_DEVICE_INVALIDATED) detail = "Dispositivo desconectado. Reconecte, atualize a lista e inicie novamente.";
    else if (hr == AUDCLNT_E_DEVICE_IN_USE) detail = "O dispositivo está em uso exclusivo por outro aplicativo.";
    else detail = QString("Erro de áudio do Windows 0x%1.").arg(quint32(hr), 8, 16, QLatin1Char('0'));
    throw std::runtime_error((QString::fromUtf8(operation) + ": " + detail).toStdString());
}
template<class T> class Ptr {
    T *p = nullptr;
public:
    Ptr() = default;
    Ptr(const Ptr &) = delete;
    Ptr &operator=(const Ptr &) = delete;
    ~Ptr() { reset(); }
    T *operator->() const { return p; }
    T *get() const { return p; }
    explicit operator bool() const { return p != nullptr; }
    void reset() { if (p) { p->Release(); p = nullptr; } }
    T **put() { reset(); return &p; }
};
class Apartment {
    HRESULT hr;
public:
    Apartment() : hr(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)) {
        if (hr != RPC_E_CHANGED_MODE) check(hr, "Inicializar áudio");
    }
    ~Apartment() { if (SUCCEEDED(hr)) CoUninitialize(); }
};
inline void enumerator(Ptr<IMMDeviceEnumerator> &e) {
    check(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
          __uuidof(IMMDeviceEnumerator), reinterpret_cast<void **>(e.put())), "Listar dispositivos");
}
inline QString id(IMMDevice *d) {
    LPWSTR value = nullptr; check(d->GetId(&value), "Identificar dispositivo");
    const auto result = QString::fromWCharArray(value); CoTaskMemFree(value); return result;
}
inline QString name(IMMDevice *d) {
    Ptr<IPropertyStore> store; check(d->OpenPropertyStore(STGM_READ, store.put()), "Ler dispositivo");
    PROPVARIANT value; PropVariantInit(&value);
    check(store->GetValue(PKEY_Device_FriendlyName, &value), "Ler nome do dispositivo");
    const auto result = value.vt == VT_LPWSTR ? QString::fromWCharArray(value.pwszVal) : id(d);
    PropVariantClear(&value); return result;
}
inline void device(IMMDeviceEnumerator *e, const QString &id, Ptr<IMMDevice> &d) {
    const auto wide = id.toStdWString();
    check(e->GetDevice(wide.c_str(), d.put()), "Abrir dispositivo selecionado");
}
inline void activate(IMMDevice *d, Ptr<IAudioClient> &client) {
    check(d->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
          reinterpret_cast<void **>(client.put())), "Abrir fluxo de áudio");
}
}

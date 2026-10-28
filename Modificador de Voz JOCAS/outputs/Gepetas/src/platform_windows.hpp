// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "devices.hpp"
#include "wasapi.hpp"

inline AudioInventory enumerateAudioDevices() {
    Wasapi::Apartment apartment;
    Wasapi::Ptr<IMMDeviceEnumerator> enumerator; Wasapi::enumerator(enumerator);
    AudioInventory result; result.virtualReady = false;
    bool cableCapture = false;
    for (const auto flow : {eCapture, eRender}) {
        Wasapi::Ptr<IMMDeviceCollection> collection;
        Wasapi::check(enumerator->EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, collection.put()), "Listar áudio");
        UINT count = 0; Wasapi::check(collection->GetCount(&count), "Contar dispositivos");
        for (UINT i = 0; i < count; ++i) {
            Wasapi::Ptr<IMMDevice> device; Wasapi::check(collection->Item(i, device.put()), "Ler dispositivo");
            AudioDevice info{Wasapi::id(device.get()), Wasapi::name(device.get())};
            const bool cableInput = info.label.contains("CABLE Input", Qt::CaseInsensitive);
            const bool cableOutput = info.label.contains("CABLE Output", Qt::CaseInsensitive);
            if (flow == eRender && cableInput) result.virtualOutput = info.id;
            else if (flow == eCapture && cableOutput) cableCapture = true;
            else if (flow == eCapture) result.inputs.append(info);
            else result.outputs.append(info);
        }
        Wasapi::Ptr<IMMDevice> def;
        if (SUCCEEDED(enumerator->GetDefaultAudioEndpoint(flow, eCommunications, def.put()))) {
            if (flow == eCapture) result.defaultInput = Wasapi::id(def.get());
            else result.defaultOutput = Wasapi::id(def.get());
        }
    }
    result.virtualReady = !result.virtualOutput.isEmpty() && cableCapture;
    if (!result.virtualReady)
        result.message = "Instale o VB-CABLE para usar a voz no Discord. Depois, clique em Atualizar.";
    return result;
}

class VirtualMic {
    QString outputId;
public:
    QString cleanupError;
    void create() {
        const auto devices = enumerateAudioDevices();
        if (!devices.virtualReady) throw std::runtime_error(devices.message.toStdString());
        outputId = devices.virtualOutput;
    }
    QString target() const { return outputId; }
    void destroy() { outputId.clear(); cleanupError.clear(); }
};
inline QString platformLabel() { return "WINDOWS  /  WASAPI"; }
inline QString platformAudioError() { return "Não consegui acessar o áudio. Verifique os dispositivos e as permissões do microfone no Windows."; }
inline QString discordSourceName() { return "CABLE Output (VB-Audio Virtual Cable)"; }
inline QString platformStoppedMessage() { return "Parado. O envio de voz para o cabo virtual foi desligado."; }

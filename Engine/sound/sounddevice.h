#pragma once

#include "AL/efx-presets.h"
#include "reverb.h"
#include <memory>
#include <vector>
#include <string>

namespace Tempest {

class Sound;
class SoundProducer;
class SoundEffect;
class IDevice;

enum class ChannelConfig : uint8_t {
  Mono,
  Stereo,
  Headphones,
  FourZero,
  FiveOne,
  SevenOne
};

class SoundDevice final {
  public:
    SoundDevice (ChannelConfig c = ChannelConfig::Stereo, uint16_t frequency=44100, uint8_t index=0, bool enableEfx=true);
    SoundDevice (const SoundDevice&) = delete;
    ~SoundDevice();

    SoundDevice& operator = ( const SoundDevice& s) = delete;

    static std::vector<std::string> enumerateDevices(bool extendedProviders, bool beautify);
    
    SoundEffect load(const char* fname);
    SoundEffect load(Tempest::IDevice& d);
    SoundEffect load(const Sound& snd);
    SoundEffect load(std::unique_ptr<SoundProducer> &&p);

    void process();
    void suspend();
    bool isClearedOfErrors();
    bool updateReverbParameters(ReverbCategory slotId,ReverbPreset preset);

    void setListenerPosition(float x,float y,float z);
    void setListenerDirection(float dx, float dy, float dz, float ux, float uy, float uz);
    void setGlobalVolume(float v);
    int  getEffectSlot(const ReverbCategory& category);
    int  getEffect    (const ReverbCategory& category);
    bool efxValid() { return isEfxInitialized; }

  private:
    struct Data;
    struct Device;

    void* context();

    std::unique_ptr<Data> data;

    static std::shared_ptr<Device> device(uint8_t deviceIndex);
    static std::shared_ptr<Device> implDevice(uint8_t deviceIndex);

    enum EffectConfig {
      NUM_AUX_SENDS=4,
      NUM_EFF_SLOTS=4,
      NUM_EFFECTS=4,
      NUM_FILTERS=1
    };

    int auxSends=0;
    bool isEfxInitialized=false;
    unsigned int uiEffectSlot[NUM_EFF_SLOTS]={0};
    unsigned int uiEffects   [NUM_EFFECTS]  ={0};

  friend class SoundEffect;
  };

}

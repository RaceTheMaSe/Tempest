#include "sounddevice.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include "AL/efx.h"
#include "reverb.h"

#include <Tempest/File>
#include <Tempest/Sound>
#include <Tempest/SoundEffect>
#include <Tempest/SoundReverb>
#include <Tempest/Except>

#include <algorithm>
#include <cstring>
#include <deque>
#include <mutex>

#include <Tempest/Log>

using namespace Tempest;

ALCint toAlChannels(ChannelConfig c) {
  switch(c) {
    case ChannelConfig::Mono:
      return ALC_MONO_SOFT;
      break;
    case ChannelConfig::Stereo:
    case ChannelConfig::Headphones:
      return ALC_STEREO_SOFT;
      break;
    case ChannelConfig::FourZero:
      return ALC_QUAD_SOFT;
      break;
    case ChannelConfig::FiveOne:
      return ALC_5POINT1_SOFT;
      break;
    case ChannelConfig::SevenOne:
      return ALC_7POINT1_SOFT;
      break;
    default:
      return ALC_STEREO_SOFT;
      break;
  }
}

std::vector<std::string> SoundDevice::enumerateDevices(bool /*extendedProviders*/, bool beautify) {
  std::vector<std::string> devicesNames;
  if (alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT") == AL_TRUE) {
    const ALCchar *allDevices = nullptr;
    const ALCchar *defaultDeviceName = nullptr;
    allDevices        = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
    defaultDeviceName = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
    ALCchar current[4096]={};
    size_t i=0;
    auto *currBegin = (ALCchar*)current;
    while (allDevices[i]!=0) {
      current[i]=allDevices[i];
      i++;
      if(allDevices[i]==0) {
        std::string device(currBegin);
        if(beautify) {
          if(device.find("HDA ")!=std::string::npos) device=device.substr(4);
          if(device.find('(')   !=std::string::npos) device=device.substr(0,device.find('(')-1);
          if(device.find(", ")  !=std::string::npos) device.replace(device.find(", "),2," - ");
        }
        // if(extendedProviders || (!extendedProviders && device.find("<YOUR_FILTER_HERE>")!=std::string::npos))
        devicesNames.emplace_back(device);
        currBegin=&current[++i];
      }
    }
  }
  else
    devicesNames.push_back(std::string(alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER)));
  return devicesNames;
}

struct SoundDevice::Data {
  std::shared_ptr<Device> dev;
  ALCcontext*             context{};
  };

#include <Tempest/Log>
struct SoundDevice::Device {
  Device(uint8_t deviceIndex) {
    const auto devices = enumerateDevices(true,false);
    dev = alcOpenDevice(deviceIndex < devices.size() ? devices[deviceIndex].c_str() : nullptr);
    int errcode=alcGetError(dev);
    if(dev==nullptr || (deviceIndex && devices.size() && errcode)) {
      Tempest::Log::e("Sound device init: \"",deviceIndex<devices.size()?devices[deviceIndex]:"<out of range index>","\": ",
        errcode?alcGetString(dev, errcode):""," - falling back to first device: ", devices[0]);
      dev = alcOpenDevice(deviceIndex < devices.size() ? devices[0].c_str() : nullptr);
      errcode=alcGetError(dev);
    }
    if(dev==nullptr || errcode)
      throw std::system_error(Tempest::SoundErrc::NoDevice);
    }
  ~Device(){
    alcCloseDevice(dev);
    }

  ALCdevice* dev=nullptr;
  };

SoundDevice::SoundDevice(ChannelConfig c, uint16_t frequency, uint8_t index, bool enableEfx):data( new Data() ) {
  data->dev = device(index);
  ALCint config[9] = { // make larger so its null terminated
    ALC_FORMAT_CHANNELS_SOFT,
    toAlChannels(c),
    ALC_FORMAT_TYPE_SOFT,
    ALC_FLOAT_SOFT,
    ALC_FREQUENCY,
    frequency,
    ALC_MAX_AUXILIARY_SENDS,
    NUM_AUX_SENDS
    };
  data->context = alcCreateContext(data->dev->dev,(ALCint*)config);
  if(data->context==nullptr)
    throw std::system_error(Tempest::SoundErrc::NoDevice);

  if(!isEfxInitialized && enableEfx) {
    // code derived from OpenAL effects extension guide tutorials 1-4
    alcMakeContextCurrent(data->context);
    bool isEfx = alcIsExtensionPresent(data->dev->dev,ALC_EXT_EFX_NAME);
    if(!isEfx) {
      Tempest::Log::i("Extended sound effects not available");
      }
    else {
      alcGetIntegerv(data->dev->dev, ALC_MAX_AUXILIARY_SENDS,1,&auxSends);
      if(auxSends<NUM_AUX_SENDS)
        Tempest::Log::i("Error in SoundDevice - effect sends not enough: ",auxSends," expected: ",NUM_AUX_SENDS,")");
      ALCenum err=alcGetError(data->dev->dev);
      if(err)
        Tempest::Log::i("Error in SoudDevice: ",alGetString(err));
      for(unsigned int& slotId:uiEffectSlot) {
        alGenAuxiliaryEffectSlotsCt(data->context,1, &slotId);
        err=alcGetError(data->dev->dev);
        if(err) {
          Tempest::Log::i("Error in SoudDevice - effect slot setup: ",alGetString(err));
          break;
          }
        }
      err=alcGetError(data->dev->dev);
      if(err) {
        Tempest::Log::i("Error in SoudDevice - effect slot setup: ",alGetString(err));
        return;
        }
      alGenEffectsCt(data->context,NUM_EFFECTS, (ALCuint*)uiEffects);
      err=alcGetError(data->dev->dev);
      if(err) {
        Tempest::Log::i("Error in SoudDevice effect setup: ",alGetString(err));
        return;
        }
      for(int slot=0;slot<NUM_EFFECTS;slot++) {
        if(alIsEffectCt(data->context,uiEffects[slot])) {
          alEffectiCt(data->context,uiEffects[slot],AL_EFFECT_TYPE,AL_EFFECT_EAXREVERB);
          err=alcGetError(data->dev->dev);
          if(err) {
            Tempest::Log::i("Error in SoudDevice reverb slot ",slot," effect setup:",alGetString(err));
            return;
            }
          switch(slot) {
            // case 0: updateParameters((ReverbCategory)slot,ReverbPreset::Generic); break; // no reverb
            case 1: //updateReverbParameters((ReverbCategory)slot,ReverbPreset::Generic); break; // sound effect
            case 2: updateReverbParameters((ReverbCategory)slot,ReverbPreset::Generic); break; // voice
            // case 3: updateParameters((ReverbCategory)slot,ReverbPreset::Generic); break; // music
            // default:updateParameters((ReverbCategory)slot,ReverbPreset::Generic); break;
            }
          err=alcGetError(data->dev->dev);
          if(err) {
            Tempest::Log::i("Error in SoudDevice reverb slot ",slot," decay setup: ",alGetString(err));
            return;
            }
          }  
        }
      //Attach effects to auxiliary effect slot
      for(size_t slot=0;slot<NUM_EFF_SLOTS;slot++) {
        alAuxiliaryEffectSlotiCt(data->context,uiEffectSlot[slot],AL_EFFECTSLOT_EFFECT, (int)uiEffects[slot]);
        err=alcGetError(data->dev->dev);
        if(err) {
          Tempest::Log::i("Error in SoudDevice effect slot ",slot," attach: ",alGetString(err));
          return;
          }
        }
      isEfxInitialized=true;
      }
    }
  // alListenerfCt(data->context, AL_METERS_PER_UNIT, 0.01f);
  alDistanceModel(data->context, AL_LINEAR_DISTANCE);
  process();
  }

SoundDevice::~SoundDevice() {
  if( data->context ){
    // if(isEfxInitialized) {
    //   alDeleteEffectsCt(data->context,NUM_EFFECTS,uiEffects); // prevents sounds to work after world switching
    //   alDeleteAuxiliaryEffectSlotsCt(data->context,NUM_EFF_SLOTS,uiEffectSlot);
    //   isEfxInitialized=false;
    // }
    alcDestroyContext(data->context);
    }
  }

SoundEffect SoundDevice::load(const char *fname) {
  Tempest::Sound s(fname);
  return load(s);
  }

SoundEffect SoundDevice::load(IDevice &d) {
  Tempest::Sound s(d);
  return load(s);
  }

SoundEffect SoundDevice::load(const Sound &snd) {
  return SoundEffect(*this,snd);
  }

SoundEffect SoundDevice::load(std::unique_ptr<SoundProducer>&& p) {
  return SoundEffect(*this,std::move(p));
  }

void SoundDevice::process() {
  alcProcessContext(data->context);
  }

void SoundDevice::suspend() {
  alcSuspendContext(data->context);
  }

bool SoundDevice::isClearedOfErrors() {
  std::string errMsg;
  static ALenum prevErr=0;
  auto lastErr = alcGetError(data->dev->dev);
  if(lastErr!=0 ) { //&& prevErr!=lastErr
    prevErr=lastErr;
    switch(lastErr) {
      case AL_INVALID_NAME:      errMsg="Invalid name";      break;
      case AL_INVALID_ENUM:      errMsg="Invalid enum";      break;
      case AL_INVALID_VALUE:     errMsg="Invalid value";     break;
      case AL_INVALID_OPERATION: errMsg="Invalid operation"; break;
      case AL_OUT_OF_MEMORY:     errMsg="Out of memory";     break;
      default: break;
    }
    Tempest::Log::e("AL lib: ",errMsg);
    return false;
    }
  return true;
  }

bool SoundDevice::updateReverbParameters(
  ReverbCategory slotId,
  ReverbPreset preset) {
  if((int)slotId>=NUM_EFFECTS)
    return false;

  auto eff = uiEffects[(int)slotId];
  auto* ctx = (ALCcontext*)context();
  EFXEAXREVERBPROPERTIES props = ReverbSettings(preset).props;

  alEffectfCt(ctx,eff,AL_EAXREVERB_DENSITY              ,props.flDensity);
  alEffectfCt(ctx,eff,AL_EAXREVERB_DIFFUSION            ,props.flDiffusion);
  alEffectfCt(ctx,eff,AL_EAXREVERB_GAIN                 ,props.flGain);
  alEffectfCt(ctx,eff,AL_EAXREVERB_GAINHF               ,props.flGainHF);
  alEffectfCt(ctx,eff,AL_EAXREVERB_GAINLF               ,props.flGainLF);
  alEffectfCt(ctx,eff,AL_EAXREVERB_DECAY_TIME           ,props.flDecayTime);
  alEffectfCt(ctx,eff,AL_EAXREVERB_DECAY_HFRATIO        ,props.flDecayHFRatio);
  alEffectfCt(ctx,eff,AL_EAXREVERB_DECAY_LFRATIO        ,props.flDecayLFRatio);
  alEffectfCt(ctx,eff,AL_EAXREVERB_REFLECTIONS_GAIN     ,props.flReflectionsGain);
  alEffectfCt(ctx,eff,AL_EAXREVERB_REFLECTIONS_DELAY    ,props.flReflectionsDelay);
  alEffectfvCt(ctx,eff,AL_EAXREVERB_REFLECTIONS_PAN     ,(float*)props.flReflectionsPan);
  alEffectfCt(ctx,eff,AL_EAXREVERB_LATE_REVERB_GAIN     ,props.flLateReverbGain);
  alEffectfCt(ctx,eff,AL_EAXREVERB_LATE_REVERB_DELAY    ,props.flLateReverbDelay);
  alEffectfvCt(ctx,eff,AL_EAXREVERB_LATE_REVERB_PAN     ,(float*)props.flLateReverbPan);
  alEffectfCt(ctx,eff,AL_EAXREVERB_ECHO_TIME            ,props.flEchoTime);
  alEffectfCt(ctx,eff,AL_EAXREVERB_ECHO_DEPTH           ,props.flEchoDepth);
  alEffectfCt(ctx,eff,AL_EAXREVERB_MODULATION_TIME      ,props.flModulationTime);
  alEffectfCt(ctx,eff,AL_EAXREVERB_MODULATION_DEPTH     ,props.flModulationDepth);
  alEffectfCt(ctx,eff,AL_EAXREVERB_AIR_ABSORPTION_GAINHF,props.flAirAbsorptionGainHF);
  alEffectfCt(ctx,eff,AL_EAXREVERB_HFREFERENCE          ,props.flHFReference);
  alEffectfCt(ctx,eff,AL_EAXREVERB_LFREFERENCE          ,props.flLFReference);
  alEffectfCt(ctx,eff,AL_EAXREVERB_ROOM_ROLLOFF_FACTOR  ,props.flRoomRolloffFactor);
  alEffectiCt(ctx,eff,AL_EAXREVERB_DECAY_HFLIMIT        ,props.iDecayHFLimit);

  const auto slot=uiEffectSlot[(int)slotId];
  alAuxiliaryEffectSlotiCt(ctx,slot,AL_EFFECTSLOT_EFFECT,(ALint)eff);
  return isClearedOfErrors();
  }

int SoundDevice::getEffectSlot(const ReverbCategory& category) {
  return (int)uiEffectSlot[(int)category];
  };

int SoundDevice::getEffect(const ReverbCategory& category) {
  return (int)uiEffects[(int)category];
  };

void SoundDevice::setListenerPosition(float x, float y, float z) {
  float xyz[]={x,y,z};
  alListenerfvCt(data->context,AL_POSITION,(ALfloat*)xyz);
  }

void SoundDevice::setListenerDirection(float dx,float dy,float dz,float ux,float uy,float uz) {
  float ori[6] = {dx,dy,dz, ux,uy,uz};
  alListenerfvCt(data->context,AL_ORIENTATION,(ALfloat*)ori);
  }

void SoundDevice::setGlobalVolume(float v) {
  float fv[]={v};
  alListenerfvCt(data->context,AL_GAIN,(ALfloat*)fv);
  }

void* SoundDevice::context() {
  return data->context;
  }

std::shared_ptr<SoundDevice::Device> SoundDevice::device(uint8_t deviceIndex) {
  static std::mutex sync;
  std::lock_guard<std::mutex> guard(sync);
  return implDevice(deviceIndex);
  }

std::shared_ptr<SoundDevice::Device> SoundDevice::implDevice(uint8_t deviceIndex) {
  static std::weak_ptr<SoundDevice::Device> val;
  if(auto v = val.lock()){
    return v;
    }
  auto vx = std::make_shared<Device>(deviceIndex);
  val = vx;
  return vx;
  }

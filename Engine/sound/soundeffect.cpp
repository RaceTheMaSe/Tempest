#include "soundeffect.h"
#include "AL/efx.h"
#include "sound/sound.h"

#include <AL/al.h>
#include <AL/alc.h>

#include <Tempest/SoundDevice>
#include <Tempest/Except>
#include <Tempest/Log>

#include <chrono>
#include <thread>
#include <atomic>

using namespace Tempest;

struct SoundEffect::Impl {
  enum {
    NUM_BUF = 3,
    BUFSZ   = 4096
    };

  Impl()=default;
  Impl(Impl&)=delete;
  Impl(Impl&&)=delete;
  Impl operator=(Impl&)=delete;
  Impl operator=(Impl&&)=delete;

  Impl(SoundDevice &dev, const Sound &src)
    :dev(&dev), data(src.data) {
    if(data==nullptr)
      return;
    ALCcontext* ctx = context();
    alGenSourcesCt(ctx, 1, &source);
    alSourceBufferCt(ctx, source, reinterpret_cast<ALbuffer*>(data->buffer));
    }

  Impl(SoundDevice &dev, std::unique_ptr<SoundProducer> &&src)
    :dev(&dev), data(nullptr), producer(std::move(src)) {
    ALCcontext* ctx = context();
    alGenSourcesCt(ctx, 1, &source);

    threadFlag.store(true);
    producerThread = std::thread([this]() noexcept {
      soundThreadFn();
      });
    }

  ~Impl(){
    if(source==0)
      return;

    ALCcontext* ctx = context();
    if(threadFlag.load()) {
      threadFlag.store(false);
      producerThread.join();
      producer.reset();
      } else {
      alDeleteSourcesCt(ctx, 1, &source);
      }
    }

  void soundThreadFn() {
    SoundProducer& src  = *producer;
    ALsizei        freq = src.frequency;
    ALenum         frm  = src.channels==2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
    auto           ctx  = context();
    ALbuffer*      qBuffer[NUM_BUF]={};
    int16_t        bufData[BUFSZ*2]={};
    ALint          zero=0;

    for(size_t i=0;i<NUM_BUF;++i){
      auto b = alNewBuffer();
      if(b==nullptr){
        for(size_t r=0;r<i;++r)
          alDelBuffer(b);
        return;
        }
      qBuffer[i] = b;
      }

    for(auto& i:qBuffer) {
      renderSound(src,(int16_t*)bufData,BUFSZ);
      alBufferDataCt(ctx, i, frm, (int16_t*)bufData, BUFSZ*sizeof(int16_t)*2, freq);
      }
    alSourceQueueBuffersCt(ctx,source,NUM_BUF,(ALbuffer**)qBuffer);
    alSourceivCt(ctx,source,AL_LOOPING,&zero);
    alSourcePlayvCt(ctx,1,&source);

    while(threadFlag.load()) {
      int bufInQueue=0, bufProcessed=0;
      alGetSourceivCt(ctx,source, AL_BUFFERS_QUEUED,    &bufInQueue);
      alGetSourceivCt(ctx,source, AL_BUFFERS_PROCESSED, &bufProcessed);

      int bufC = (bufInQueue-bufProcessed);
      if(bufC>2){
        //alWait(ctx); // uncommented for Android and reverb effect extension - not to wait may be wrong but lets see ... this wait blocks the program from switching worlds and exiting and receiving a signal to unlock
        continue;
        }

      if(paused.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        continue;
        }

      for(int i=0;i<bufProcessed;++i) {
        ALbuffer* nextBuffer=nullptr;
        alSourceUnqueueBuffersCt(ctx,source,1,&nextBuffer);

        if(nextBuffer==nullptr)
          break;
        if(paused.load())
          break;

        renderSound(src,(int16_t*)bufData,BUFSZ);
        alBufferDataCt(ctx, nextBuffer, frm, (int16_t*)bufData, BUFSZ*sizeof(int16_t)*2, freq);
        alSourceQueueBuffersCt(ctx,source,1,&nextBuffer);
        }

      // alGetSourceivCt(ctx,source,AL_SOURCE_STATE,&state);
      }

    alSourcePausevCt(ctx,1,&source);
    alDeleteSourcesCt(ctx, 1, &source);
    for(auto& i:qBuffer)
      alDelBuffer(i);
    source=0;
    }

  void renderSound(SoundProducer& src,int16_t* data,size_t sz) noexcept {
    src.renderSound(data,sz);
    }

  void setPause(bool b) {
    paused.store(b);
    }

  ALCcontext* context(){
    return reinterpret_cast<ALCcontext*>(dev->context());
    }

  SoundDevice*                   dev    = nullptr;
  std::shared_ptr<Sound::Data>   data;
  uint32_t                       source = 0;
  ReverbCategory                 category=ReverbCategory::NoReverb;

  std::thread                    producerThread;
  std::atomic_bool               threadFlag={false};
  std::atomic_bool               paused    ={false};
  std::unique_ptr<SoundProducer> producer;
  };

SoundProducer::SoundProducer(uint16_t frequency, uint16_t channels)
  :frequency(frequency),channels(channels){
  if(channels!=1 && channels!=2)
    throw std::system_error(Tempest::SoundErrc::InvalidChannelsCount);
  }

SoundEffect::SoundEffect(SoundDevice &dev, const Sound &src)
  :impl(new Impl(dev,src)) {
  }

SoundEffect::SoundEffect(SoundDevice &dev, std::unique_ptr<SoundProducer> &&src)
  :impl(new Impl(dev,std::move(src))) {
  }

SoundEffect::SoundEffect()
  :impl(new Impl()){
  }

SoundEffect::SoundEffect(Tempest::SoundEffect &&s) noexcept
  :impl(std::move(s.impl)) {
  }

SoundEffect::~SoundEffect() = default;

SoundEffect &SoundEffect::operator=(Tempest::SoundEffect &&s) noexcept {
  std::swap(impl,s.impl);
  return *this;
  }

void SoundEffect::play() {
  if(impl->source==0 || !impl->context())
    return;
  ALCcontext* ctx = impl->context();
  impl->setPause(false);
  alSourcePlayvCt(ctx,1,&impl->source);
  }

void SoundEffect::pause() {
  if(impl->source==0 || !impl->context())
    return;
  ALCcontext* ctx = impl->context();
  impl->setPause(true);
  alSourcePausevCt(ctx,1,&impl->source);
  }

bool SoundEffect::isEmpty() const {
  return impl->source==0;
  }

bool SoundEffect::isFinished() const {
  if(impl->source==0 || !impl->context())
    return true;
  int32_t state=0;
  ALCcontext* ctx = impl->context();
  alGetSourceivCt(ctx,impl->source,AL_SOURCE_STATE,&state);
  return state==AL_STOPPED || state==AL_INITIAL;
  }

uint64_t Tempest::SoundEffect::timeLength() const {
  if(impl->data)
    return impl->data->timeLength();
  return 0;
  }

uint64_t Tempest::SoundEffect::currentTime() const {
  if(impl->source==0 || !impl->context())
    return 0;
  float result=0;
  ALCcontext* ctx = impl->context();
  alGetSourcefvCt(ctx, impl->source, AL_SEC_OFFSET, &result);
  return uint64_t(result*1000);
  }

void SoundEffect::setPosition(float x, float y, float z) {
  float p[3]={x,y,z};
  if(impl->source==0 || !impl->context())
    return;
  ALCcontext* ctx = impl->context();
  alSourcefvCt(ctx, impl->source, AL_POSITION, (ALfloat*)p);
  }

void SoundEffect::setDirection(float x, float y, float z) {
  float p[3]={x,y,z};
  if(impl->source==0 || !impl->context())
    return;
  ALCcontext* ctx = impl->context();
  alSourcefvCt(ctx, impl->source, AL_DIRECTION, (ALfloat*)p);
  }

void SoundEffect::setVelocity(float x, float y, float z) {
  float p[3]={x,y,z};
  if(impl->source==0 || !impl->context())
    return;
  ALCcontext* ctx = impl->context();
  alSourcefvCt(ctx, impl->source, AL_VELOCITY, (ALfloat*)p);
  }

std::array<float,3> SoundEffect::position() const {
  std::array<float,3> ret={};
  if(impl->source==0 || !impl->context())
    return ret;
  ALCcontext* ctx = impl->context();
  alGetSourcefvCt(ctx,impl->source,AL_POSITION,&ret[0]);
  return ret;
  }

float SoundEffect::x() const {
  return position()[0];
  }

float SoundEffect::y() const {
  return position()[1];
  }

float SoundEffect::z() const {
  return position()[2];
  }

void SoundEffect::setMaxDistance(float dist) {
  if(impl->source==0 || !impl->context())
    return;
  ALCcontext* ctx = impl->context();
  alSourcefvCt(ctx, impl->source, AL_MAX_DISTANCE, &dist);
  }

void SoundEffect::setRefDistance(float dist) {
  if(impl->source==0 || !impl->context())
    return;
  ALCcontext* ctx = impl->context();
  alSourcefvCt(ctx, impl->source, AL_REFERENCE_DISTANCE, &dist);
  }

void SoundEffect::setVolume(float val) {
  if(impl->source==0 || !impl->context())
    return;
  ALCcontext* ctx = impl->context();
  alSourcefvCt(ctx, impl->source, AL_GAIN, &val);
  }

void SoundEffect::setPitch(float val) {
  if(impl->source==0 || !impl->context())
    return;
  val = std::pow(2.f,val);
  ALCcontext* ctx = impl->context();
  alSourcefvCt(ctx, impl->source, AL_PITCH, &val);
  }

void SoundEffect::setInnerConeAngle(float val) {
  if(impl->source==0 || !impl->context())
    return;
  ALCcontext* ctx = impl->context();
  alSourcefvCt(ctx, impl->source, AL_CONE_INNER_ANGLE, &val);
  }

void SoundEffect::setOuterConeAngle(float val) {
  if(impl->source==0 || !impl->context())
    return;
  ALCcontext* ctx = impl->context();
  alSourcefvCt(ctx, impl->source, AL_CONE_OUTER_ANGLE, &val);
  }

void SoundEffect::setOuterConeGain(float val) {
  if(impl->source==0 || !impl->context())
    return;
  ALCcontext* ctx = impl->context();
  alSourcefvCt(ctx, impl->source, AL_CONE_OUTER_GAIN, &val);
  }

float SoundEffect::volume() const {
  if(impl->source==0 || !impl->context())
    return 0;
  float val=0;
  ALCcontext* ctx = impl->context();
  alGetSourcefvCt(ctx, impl->source, AL_GAIN, &val);
  return val;
  }

void SoundEffect::setReverb(float val) {
  if(impl->source==0 || !impl->dev) {
    Log::e("Sound device invalid while accessing reverb");
    return;
    }
  if(!impl->dev->efxValid())
    return;
  ALCcontext* ctx = impl->context();
  // FIXME: with this change to the OpenAL library the reverb can be set on each source individually - not sure if this is how its supposed to be
  alSourcefvCt(ctx, impl->source, AL_AUXILIARY_SEND_FILTER_GAIN, &val);
  alSourcefvCt(ctx, impl->source, AL_AUXILIARY_SEND_FILTER_GAINHF, &val);
  return;
  }

void SoundEffect::setReverbDecay(float val) {
  if(impl->source==0 || !impl->dev) {
    Log::e("Sound device invalid while accessing reverb decay");
    return;
    }
  if(!impl->dev->efxValid())
    return;
  ALCcontext* ctx = impl->context();
  const auto slot=impl->dev->getEffectSlot(impl->category);
  const auto eff =impl->dev->getEffect(impl->category);
  float baseVal=1.f;
  alEffectfCt(ctx,eff,AL_EAXREVERB_DECAY_TIME,baseVal * val);
  alAuxiliaryEffectSlotiCt(ctx,slot,AL_EFFECTSLOT_EFFECT,eff);
  }

void SoundEffect::setReverbCategory(ReverbCategory category) {
  impl->category = category;
  if(!impl->dev) {
    Log::e("Sound device invalid while accessing reverb category");
    return;
  }
  if(!impl->dev->efxValid())
    return;
  const auto slot=impl->dev->getEffectSlot(category);
  ALCcontext* ctx = impl->context();
  alSource3iCt(ctx,impl->source,AL_AUXILIARY_SEND_FILTER,slot,0,0);
}

ReverbCategory SoundEffect::getReverbCategory() const {
  return impl->category;
  }

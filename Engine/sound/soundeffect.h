#pragma once

#include <Tempest/Sound>
#include <Tempest/SoundReverb>
#include <cstdint>
#include <array>

namespace Tempest {

class SoundDevice;

class SoundProducer {
  public:
    SoundProducer(uint16_t frequency,uint16_t channels);
    SoundProducer(SoundProducer&) =default;
    SoundProducer(SoundProducer&&)=delete;
    virtual ~SoundProducer()=default;

    SoundProducer& operator=(SoundProducer&) =delete;
    SoundProducer& operator=(SoundProducer&&)=delete;

    virtual void renderSound(int16_t* out,size_t n) = 0;

  private:
    uint16_t frequency = 44100;
    uint16_t channels  = 2;
  friend class SoundEffect;
  };

class SoundEffect final {
  public:
    SoundEffect();
    SoundEffect(SoundEffect&)=delete;
    SoundEffect(SoundEffect&& s) noexcept;
    ~SoundEffect();

    SoundEffect& operator=(SoundEffect&)=delete;
    SoundEffect& operator=(SoundEffect&& s) noexcept;

    void     play();
    void     pause();

    bool     isEmpty()     const;
    bool     isFinished()  const;
    uint64_t timeLength()  const;
    uint64_t currentTime() const;

    void     setPosition(float x,float y,float z);
    void     setDirection(float x,float y,float z);
    void     setVelocity(float x,float y,float z);
    void     setMaxDistance(float dist);
    void     setRefDistance(float dist);
    void     setVolume(float val);
    void     setPitch(float val);
    void     setInnerConeAngle(float val);
    void     setOuterConeAngle(float val);
    void     setOuterConeGain(float val);
    float    volume() const;
    void     setReverb(float val);
    void     setReverbDecay(float val);
    void     setReverbCategory(ReverbCategory category);
    ReverbCategory getReverbCategory() const;

    std::array<float,3> position() const;
    float    x() const;
    float    y() const;
    float    z() const;

  private:
    SoundEffect(SoundDevice& dev,const Sound &src);
    SoundEffect(SoundDevice& dev,std::unique_ptr<SoundProducer>&& src);

    struct Impl;
    std::unique_ptr<Impl> impl;

  friend class SoundDevice;
  };

}

#pragma once

#include "AL/efx-presets.h"
#include <cstdint>
#include <string>

namespace Tempest {
enum class ReverbCategory {
  NoReverb = 0,
  Sfx      = 1,
  Voice    = 2,
  Music    = 3
  };

enum class ReverbPreset : uint8_t {
  Off=0,
  OpenGothicDefault,
  Generic,
  PaddedCell,
  Room,
  Bathroom,
  LivingRoom,
  StoneRoom,
  Auditorium,
  ConcertHall,
  Cave,
  Arena,
  Hangar,
  CarpetedHallway,
  StoneCorridor,
  Alley,
  Forest,
  City,
  Mountains,
  Quarry,
  Plain,
  ParkingLot,
  SewerPipe,
  Underwater,
  Drugged,
  Dizzy,
  Psychotic,
  WoodenGalleonSmallRoom,
  WoodenGalleonRoom,
  WoodenGalleonLargeRoom,
  Canyon
  };

struct ReverbSettings {
  ReverbSettings() =default;
  ReverbSettings(const Tempest::ReverbPreset& preset);

  const std::string name() const;
  Tempest::ReverbPreset preset = Tempest::ReverbPreset::OpenGothicDefault;
  EFXEAXREVERBPROPERTIES props={};
  float level = 1.0f;
  };
}

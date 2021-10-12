#include "reverb.h"
#include "AL/efx-presets.h"

using namespace Tempest;

ReverbSettings::ReverbSettings(const ReverbPreset& preset):preset(preset) {
  switch (preset) { 
    case ReverbPreset::Off:                    props = { 1.0f, 1.0f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, {0.f, 0.f, 0.f}, 0.f, 0.f, { 0.f, 0.f, 0.f }, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0 }; break;
    case ReverbPreset::OpenGothicDefault: {
      props = EFX_REVERB_PRESET_GENERIC;
      props.flGain      = 0.1162f;
      props.flGainHF    = 0.5913f;
      props.flDecayTime = 0.2000f;
      break;
      }
    case ReverbPreset::Generic:                props = EFX_REVERB_PRESET_GENERIC;             break;
    case ReverbPreset::PaddedCell:             props = EFX_REVERB_PRESET_PADDEDCELL;          break;
    case ReverbPreset::Room:                   props = EFX_REVERB_PRESET_ROOM;                break;
    case ReverbPreset::Bathroom:               props = EFX_REVERB_PRESET_BATHROOM;            break;
    case ReverbPreset::LivingRoom:             props = EFX_REVERB_PRESET_LIVINGROOM;          break;
    case ReverbPreset::StoneRoom:              props = EFX_REVERB_PRESET_STONEROOM;           break;
    case ReverbPreset::Auditorium:             props = EFX_REVERB_PRESET_AUDITORIUM;          break;
    case ReverbPreset::ConcertHall:            props = EFX_REVERB_PRESET_CONCERTHALL;         break;
    case ReverbPreset::Cave:                   props = EFX_REVERB_PRESET_CAVE;                break;
    case ReverbPreset::Arena:                  props = EFX_REVERB_PRESET_ARENA;               break;
    case ReverbPreset::Hangar:                 props = EFX_REVERB_PRESET_HANGAR;              break;
    case ReverbPreset::CarpetedHallway:        props = EFX_REVERB_PRESET_CARPETEDHALLWAY;     break;
    case ReverbPreset::StoneCorridor:          props = EFX_REVERB_PRESET_STONECORRIDOR;       break;
    case ReverbPreset::Alley:                  props = EFX_REVERB_PRESET_ALLEY;               break;
    case ReverbPreset::Forest:                 props = EFX_REVERB_PRESET_FOREST;              break;
    case ReverbPreset::City:                   props = EFX_REVERB_PRESET_CITY;                break;
    case ReverbPreset::Mountains:              props = EFX_REVERB_PRESET_MOUNTAINS;           break;
    case ReverbPreset::Quarry:                 props = EFX_REVERB_PRESET_QUARRY;              break;
    case ReverbPreset::Plain:                  props = EFX_REVERB_PRESET_PLAIN;               break;
    case ReverbPreset::ParkingLot:             props = EFX_REVERB_PRESET_PARKINGLOT;          break;
    case ReverbPreset::SewerPipe:              props = EFX_REVERB_PRESET_SEWERPIPE;           break;
    case ReverbPreset::Underwater:             props = EFX_REVERB_PRESET_UNDERWATER;          break;
    case ReverbPreset::Drugged:                props = EFX_REVERB_PRESET_DRUGGED;             break;
    case ReverbPreset::Dizzy:                  props = EFX_REVERB_PRESET_DIZZY;               break;
    case ReverbPreset::Psychotic:              props = EFX_REVERB_PRESET_PSYCHOTIC;           break;
    case ReverbPreset::WoodenGalleonSmallRoom: props = EFX_REVERB_PRESET_WOODEN_SMALLROOM;    break;
    case ReverbPreset::WoodenGalleonRoom:      props = EFX_REVERB_PRESET_WOODEN_MEDIUMROOM;   break;
    case ReverbPreset::WoodenGalleonLargeRoom: props = EFX_REVERB_PRESET_WOODEN_LONGPASSAGE;  break;
    case ReverbPreset::Canyon:                 props = EFX_REVERB_PRESET_OUTDOORS_DEEPCANYON; break;

    default:                                   props = EFX_REVERB_PRESET_GENERIC;             break;
    }
  };

static const char* ReverbPresetNames[] = {
  "Off",
  "OpenGothicDefault",
  "Generic",
  "PaddedCell",
  "Room",
  "Bathroom",
  "LivingRoom",
  "StoneRoom",
  "Auditorium",
  "ConcertHall",
  "Cave",
  "Arena",
  "Hangar",
  "CarpetedHallway",
  "StoneCorridor",
  "Alley",
  "Forest",
  "City",
  "Mountains",
  "Quarry",
  "Plain",
  "ParkingLot",
  "SewerPipe",
  "Underwater",
  "Drugged",
  "Dizzy",
  "Psychotic",
  "WoodenGalleonSmallRoom",
  "WoodenGalleonRoom",
  "WoodenGalleonLargeRoom",
  "Canyon"
};

const std::string ReverbSettings::name() const {
  return ReverbPresetNames[size_t(preset)];
  }

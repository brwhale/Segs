/*
 * SEGS - Super Entity Game Server
 * http://www.segs.io/
 * Copyright (c) 2006 - 2018 SEGS Team (see AUTHORS.md)
 * This software is licensed under the terms of the 3-clause BSD License. See LICENSE.md for details.
 */

#pragma once

#include <stdint.h>

class PowerTrayItem;
class PowerTray;
class PowerTrayGroup;
struct PlayerData;

template<class Archive>
void serialize(Archive &archive, PlayerData &pd, uint32_t const version);

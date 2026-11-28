#pragma once

#include <Config/ConfigVariable.h>

namespace fly_vars {
    CONFIG_VARIABLE(Enabled, bool, false);
    CONFIG_VARIABLE(SpeedMultiplier, float, 1.0f);
    CONFIG_VARIABLE(FlyKey, int, 0); // 0=Shift, 1=Ctrl, 2=Alt
}

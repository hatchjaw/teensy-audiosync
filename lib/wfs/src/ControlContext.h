#ifndef WFSCONTROLCONTEXT_H
#define WFSCONTROLCONTEXT_H

#include <string>
#include <unordered_map>
#include <AnanasUtils.h>

namespace ananas::WFS
{
    using SourcePositionsMap = std::unordered_map<std::string, SmoothedValue<float> >;

    struct ControlContext
    {
        ListenableParameter<int> moduleID;
        ListenableParameter<float> speakerSpacing;
        SourcePositionsMap sourcePositions;
    };
};

#endif //WFSCONTROLCONTEXT_H

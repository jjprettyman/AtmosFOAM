#ifndef VELOCITY_PROFILE_H
#define VELOCITY_PROFILE_H

#include "fvCFD.H"

class VelocityProfile
{
    public:
    virtual vector velocityAt(const point& p) const = 0;
    virtual ~VelocityProfile() {};
};

#endif
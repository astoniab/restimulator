#include "posekeypointdata.h"

std::array<std::string, PoseTrackingLocation::POSE_TRACKING_MAX> PoseTrackingLocationName{ "None","Head","Hips","Left Hand","Right Hand","Left Foot","Right Foot" };

bool KeypointPosition::operator==(const KeypointPosition& val) const
{
    return (m_x == val.m_x && m_y == val.m_y && m_z == val.m_z);
}

bool KeypointPosition::operator!=(const KeypointPosition& val) const
{
    return !(*this == val);
}

bool KeypointPosition::operator<(const KeypointPosition& val) const
{
    return (m_y < val.m_y || (m_y == val.m_y && m_x < val.m_x) || (m_y == val.m_y && m_x == val.m_x && m_z < val.m_z));
}

bool KeypointPosition::operator>(const KeypointPosition& val) const
{
    return val < *this;
}

bool KeypointPosition::operator<=(const KeypointPosition& val) const
{
    return !(*this > val);
}

bool KeypointPosition::operator>=(const KeypointPosition& val) const
{
    return !(*this < val);
}

KeypointPosition& KeypointPosition::operator+=(const KeypointPosition& rhs)
{
    *this = *this + rhs;
    return *this;
}

KeypointPosition KeypointPosition::operator+(const KeypointPosition& rhs) const
{
    KeypointPosition r = *this;

    r.m_x += rhs.m_x;
    r.m_y += rhs.m_y;
    r.m_z += rhs.m_z;

    return r;
}

KeypointPosition& KeypointPosition::operator/=(const float val)
{
    *this = *this / val;
    return *this;
}

KeypointPosition KeypointPosition::operator/(const float val) const
{
    KeypointPosition r = *this;

    if (val != 0)
    {
        r.m_x /= val;
        r.m_y /= val;
        r.m_z /= val;
    }

    return r;
}

float KeypointPosition::Distance2(const KeypointPosition& rhs) const
{
    return ((m_x - rhs.m_x) * (m_x - rhs.m_x)) + ((m_y - rhs.m_y) * (m_y - rhs.m_y)) + ((m_z - rhs.m_z) * (m_z - rhs.m_z));
}

float KeypointPosition::Distance(const KeypointPosition& rhs) const
{
    return sqrtf(Distance2(rhs));
}

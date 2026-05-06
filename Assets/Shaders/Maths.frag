const float PI = 3.14159265359;
const float EPSILON = 1e-5;

float saturate(float x)
{
    return clamp(x, 0.0, 1.0);
}

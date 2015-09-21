#ifndef RADIUMENGINE_MATH_HPP
#define RADIUMENGINE_MATH_HPP

#include <Core/RaCore.hpp>

#ifdef OS_WINDOWS
#define _USE_MATH_DEFINES
#else
#endif
#include <cmath>
#include <algorithm>
namespace Ra
{
    namespace Core
    {
        namespace Math
        {
            /// Mathematical constants casted to Scalar.
            constexpr Scalar Pi     = Scalar( 3.141592653589793 );
            constexpr Scalar InvPi  = Scalar( 1.0 / Pi );
            constexpr Scalar PiDiv2 = Scalar( Pi / 2.0 );
            constexpr Scalar PiDiv4 = Scalar( Pi / 4.0 );
            constexpr Scalar PiMul2 = Scalar( Pi * 2.0 );
            constexpr Scalar toRad  = Scalar( Pi / 180.0 );
            constexpr Scalar toDeg  = Scalar( 180 * InvPi );

            /// Useful functions
            inline constexpr Scalar toRadians( Scalar a );
            inline constexpr Scalar toDegrees( Scalar a );

            /// Integer power functions. Work for all numeric types which support
            /// multiplication and for which T(1) is a valid expression.
            /// x^0 always return T(1) and x^1 always return x (even when x is 0).

            /// Run-time exponent version.
            template<typename T> inline T ipow( const T& x, uint exp );
            /// Compile-time exponent version.
            template<uint N, typename T> inline constexpr T ipow( const T& x );

            /// Returns the sign of any numeric type as { -1, 0, 1}
            template<typename T> inline constexpr int sign( const T& val );

            /// Returns value v clamped between bounds min and max.
            template <typename T> inline constexpr T clamp( T v, T min, T max );

        } // namespace Math
    }
}

#include <Core/Math/Math.inl>

#endif // RADIUMENGINE_MATH_HPP

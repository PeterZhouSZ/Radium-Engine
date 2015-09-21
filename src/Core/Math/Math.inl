#include <Core/Math/Math.hpp>

namespace Ra
{
    namespace Core
    {
        namespace Math
        {
            inline constexpr Scalar toRadians( Scalar a )
            {
                return toRad * a;
            }

            inline constexpr Scalar toDegrees( Scalar a )
            {
                return toDeg * a ;
            }

            template<typename T> inline T ipow( const T& x, uint exp )
            {
                if ( exp == 0 )
                {
                    return T( 1 );
                }
                if ( exp == 1 )
                {
                    return x;
                }
                T p = ipow( x, exp / 2 );
                if ( ( exp  % 2 ) == 0 )
                {
                    return p * p;
                }
                else
                {
                    return p * p * x;
                }
            }

            /// This helper class is needed because C++ doesn't support function template
            /// partial specialization.
            namespace
            {
                template<typename T, uint N>
                struct IpowHelper
                {
                    static inline constexpr T pow( const T& x )
                    {
                        return ( N % 2 == 0 ) ? IpowHelper < T, N / 2 >::pow( x ) * IpowHelper < T, N / 2 >::pow( x )
                               : IpowHelper < T, N / 2 >::pow( x ) * IpowHelper < T, N / 2 >::pow( x ) * x;
                    }
                };

                template<typename T>
                struct IpowHelper<T, 1>
                {
                    static inline constexpr T pow( const T& x )
                    {
                        return x;
                    }
                };

                template<typename T>
                struct IpowHelper<T, 0>
                {
                    static inline constexpr T pow( const T& x )
                    {
                        return T( 1 );
                    }
                };

            }

            // Nb : T is last for automatic template argument deduction.
            template <uint N, typename T>
            inline constexpr T ipow( const T& x )
            {
                return IpowHelper<T, N>::pow( x );
            }

            template <typename T>
            inline constexpr int sign( const T& val )
            {
                return ( T( 0 ) < val ) - ( val < T( 0 ) );
            }

            template <typename T>
            inline constexpr T clamp( T v, T min, T max )
            {
                return std::max( min, std::min( v, max ) );
            }
        }
    }
}
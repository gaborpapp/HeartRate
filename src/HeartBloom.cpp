/*
 Copyright (C) 2012 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include "cinder/app/App.h"
#include "cinder/Cinder.h"
#include "cinder/CinderMath.h"

#include "HeartBloom.h"

using namespace ci;
using namespace ci::app;

HeartBloom::HeartBloom( int w, int h ) :
	mStrengths( ci::Channel32f( w, 1 ) ),
	mHBloom( mndl::gl::fx::KawaseHBloom( w, h ) )
{
}

float HeartBloom::gauss( float sig, float mu, float x )
{
	return 1. / ( sig * sqrt( 2. * M_PI ) ) * exp( - 3. * ( x - mu ) * ( x - mu ) / ( 2. * sig * sig ) );
}

float HeartBloom::smoothstep( float edge0, float edge1, float x )
{
	x = math< float >::clamp( ( x - edge0 ) / ( edge1 - edge0 ) );
	return x * x * ( 3 - 2 * x );
}

ci::gl::Texture &HeartBloom::process( const ci::gl::Texture &source, int iterations,
		float strength0, float strength1 )
{
	float *s = mStrengths.getData();
	float t = 0.f;
	float step = 1.f / mStrengths.getWidth();
	for ( size_t i = 0; i < mStrengths.getWidth(); i++, t += step )
	{
		float b0 = 2 * strength0 * gauss( .9, .25, t );
		b0 = ci::math< float >::pow( b0, 2.f );
		float b1 = strength1 * gauss( .4, .70, t );
		b1 = ci::math< float >::pow( b1, 2.f );
		*s = lerp( b0, b1, smoothstep( .2, .8, t ) );
		//*s = ci::math< float >::abs( ci::math< float >::sin( t + i * .01f ) ) * strength0;
		//*s = ci::math< float >::pow( *s, 2.f );
		s++;
	}
	return mHBloom.process( source, iterations, mStrengths );
}


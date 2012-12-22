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

#pragma once

#include "KawaseHBloom.h"

class HeartBloom
{
	public:
		HeartBloom() {}
		HeartBloom( int w, int h );

		ci::gl::Texture &process( const ci::gl::Texture &source, int iterations,
				float strength0, float strength1 );

		const ci::Channel32f &getStrenghtTexture() const { return mStrengths; }

	protected:
		float gauss( float sig, float mu, float x );
		float smoothstep( float edge0, float edge1, float x );

		ci::Channel32f mStrengths;
		mndl::gl::fx::KawaseHBloom mHBloom;
};


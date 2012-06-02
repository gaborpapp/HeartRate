#include "cinder/gl/gl.h"
#include "cinder/Easing.h"
#include "cinder/CinderMath.h"

#include "HeartShape.h"

using namespace ci;

HeartShape::HeartShape() :
	mColorInactive( Color::hex( 0x80800000 ) ),
	mColorActive( ColorA::hex( 0xffff0000 ) ),
	mColorInactiveOutline( Color::hex( 0x80808080 ) ),
	mColorActiveOutline( ColorA::hex( 0xff808080 ) ),
	mMinSize( 1.f ),
	mMaxSize( 2.5f ),
	mNumSegments( 100 ),
	mOutlineWidth( 2.f )
{
}

void HeartShape::resize( app::ResizeEvent event )
{
	mAreaSize = event.getSize();
}

void HeartShape::draw()
{
	float x, y, st;
	Vec2f v;
	float minSize = mAreaSize.y * mMinSize * .015;
	float maxSize = mAreaSize.y * mMaxSize * .015;

	Vec2f center = mAreaSize / 2.f;

	float distV = math<float>::abs(
			math<float>::sin( app::getElapsedSeconds() * .5) );

	glLineWidth( mOutlineWidth );

	gl::color( mColorActiveOutline );

	gl::begin( GL_LINE_LOOP );

	// right
	for ( float t = 0; t < M_PI; t += 2 * M_PI / mNumSegments )
	{
		st = math<float>::sin( t );
		x = 16 * st * st * st;
		y = -13 * math<float>::cos( t ) +
			5 * math<float>::cos( 2 * t ) +
			2 * math<float>::cos( 3 * t ) +
			math<float>::cos( 4 * t );

		v = Vec2f( x, y ) * minSize + center;
		gl::vertex( v );
	}

	// left
	for ( float t = M_PI; t < M_PI * 2; t += 2 * M_PI / mNumSegments )
	{
		st = math<float>::sin( t );
		x = 16 * st * st * st;
		y = -13 * math<float>::cos( t ) +
			5 * math<float>::cos( 2 * t ) +
			2 * math<float>::cos( 3 * t ) +
			math<float>::cos( 4 * t );

		float u = lmap( t, (float)M_PI, 2.f * (float)M_PI, 0.f, 1.f );
		float distort = distV * easeOutQuad( u ) * ( 1.f - easeInAtan( u ) );

		float sc = lerp( minSize, maxSize, distort );
		v = Vec2f( x, y ) * sc + center;
		gl::vertex( v );
	}
	gl::end();
	glLineWidth( 1.f );
}


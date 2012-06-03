#pragma once

#include <vector>

#include "cinder/Color.h"
#include "cinder/Vector.h"
#include "cinder/app/App.h"

class HeartShape
{
	public:
		HeartShape();

		void draw();

		void resize( ci::app::ResizeEvent event );

		void setMinSize( float minSize ) { mMinSize = minSize; }
		void setMaxSize( float maxSize ) { mMaxSize = maxSize; }

		void setColorInactive( const ci::ColorA &color ) { mColorInactive = color; }
		void setColorActive( const ci::ColorA &color ) { mColorActive = color; }
		void setColorInactiveOutline( const ci::ColorA &color ) { mColorInactiveOutline = color; }
		void setColorActiveOutline( const ci::ColorA &color ) { mColorActiveOutline = color; }

		void setNumSegments( int numSegments ) { mNumSegments = numSegments; }
		void setOutlineWidth( float width ) { mOutlineWidth = width; }

	protected:
		ci::ColorA mColorInactive;
		ci::ColorA mColorActive;
		ci::ColorA mColorInactiveOutline;
		ci::ColorA mColorActiveOutline;

		float mMinSize;
		float mMaxSize;

		int mNumSegments;
		float mOutlineWidth;

		ci::Vec2i mAreaSize;

		std::vector< ci::Vec2f > mPoints;
};


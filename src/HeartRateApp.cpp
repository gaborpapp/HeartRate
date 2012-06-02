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

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"

#include "cinder/Xml.h"

#include "HeartShape.h"
#include "PParams.h"


using namespace ci;
using namespace ci::app;
using namespace std;

class HeartRateApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();
		void shutdown();

		void keyDown( KeyEvent event );
		void resize( ResizeEvent event );

		void update();
		void draw();

	private:
		HeartShape mHeart;

		params::PInterfaceGl mParams;
		ColorA mColorActiveOutline;
};

void HeartRateApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 640, 480 );
}

void HeartRateApp::setup()
{
	gl::disableVerticalSync();

	// params
	fs::path paramsXml = getResourcePath() / "params.xml";
	params::PInterfaceGl::load( paramsXml );

	mParams = params::PInterfaceGl( "Parameters", Vec2i( 300, 600 ) );
	mParams.addPersistentSizeAndPosition();

	mParams.addText( "Heart shape" );
    mParams.addPersistentParam( "Outline active color", &mColorActiveOutline, ColorA::hex( 0xff808080 ) );
}

void HeartRateApp::shutdown()
{
	params::PInterfaceGl::save();
}

void HeartRateApp::resize( ResizeEvent event )
{
	mHeart.resize( event );
}

void HeartRateApp::keyDown( KeyEvent event )
{
	if ( event.getChar() == 'f' )
		setFullScreen( !isFullScreen() );
	if ( event.getCode() == KeyEvent::KEY_ESCAPE )
		quit();
}

void HeartRateApp::update()
{
	mHeart.setColorActiveOutline( mColorActiveOutline );
}

void HeartRateApp::draw()
{
	gl::clear( Color::black() );

	mHeart.draw();

	params::InterfaceGl::draw();
}

CINDER_APP_BASIC( HeartRateApp, RendererGl( RendererGl::AA_NONE ) )


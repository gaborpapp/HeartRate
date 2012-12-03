/*
 Copyright (C) 2012 Gabor Papp, Gabor Botond Barna

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

#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"

#include "cinder/Arcball.h"
#include "cinder/Camera.h"
#include "cinder/Cinder.h"

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
		void mouseDown( MouseEvent event );
		void mouseDrag( MouseEvent event );

		void resize( ResizeEvent event );

		void update();
		void draw();

	private:
		HeartShape mHeart;

		Arcball mArcball;
		CameraPersp mCamera;

		params::PInterfaceGl mParams;

		// heart shape
		ColorA mColorInactive;
		ColorA mColorActive;
		float mDisplaceScale;
		bool mTextureEnabled;
		bool mDrawDisplace;

		// debug
		float mFps;
};

void HeartRateApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1024, 768 );
}

void HeartRateApp::setup()
{
	gl::disableVerticalSync();

	// params
	params::PInterfaceGl::load( "params.xml" );

	mParams = params::PInterfaceGl( "Parameters", Vec2i( 300, 600 ) );
	mParams.addPersistentSizeAndPosition();

	// heart shape
	mParams.addText( "Heart shape" );
	//mParams.addPersistentParam( "Active color", &mColorActive, ColorA::hexA( 0xffab0706 ) );
	mParams.addPersistentParam( "Active color", &mColorActive, ColorA::white() );
	mParams.addPersistentParam( "Inactive color", &mColorInactive, ColorA::hexA( 0xff862c2c ) );
	mParams.addPersistentParam( "Displace scale", &mDisplaceScale, 15.f, "min=0 max=50" );
	mParams.addPersistentParam( "Texture enable", &mTextureEnabled, true );
	mParams.addPersistentParam( "Draw displace map", &mDrawDisplace, false );

	// heart behaviour

	mHeart.setup();

    // set up the arcball
    mArcball = Arcball( getWindowSize() );
    mArcball.setRadius( getWindowWidth() * 0.5f );

	// set up the camera
	mCamera.setPerspective( 60.f, getWindowAspectRatio(), 0.1f, 1000.0f );
	mCamera.lookAt( Vec3f( 0.f, -30.f, -200.f ), Vec3f( 0.0f, -30.f, 0.0f ) );

	// debug
	mParams.addSeparator();
	mFps = 0;
	mParams.addParam( "Fps", &mFps, "", false );
}

void HeartRateApp::shutdown()
{
	params::PInterfaceGl::save();
}

void HeartRateApp::update()
{
	mHeart.setColorActive( mColorActive );
	mHeart.setColorInactive( mColorInactive );
	mHeart.setDisplacementScale( mDisplaceScale );
	mHeart.enableTexture( mTextureEnabled );
	mHeart.update();

	mFps = getAverageFps();
}

void HeartRateApp::draw()
{
	gl::clear( Color::black() );

	gl::setViewport( getWindowBounds() );
	gl::setMatrices( mCamera );
	gl::multModelView( mArcball.getQuat() );

	mHeart.draw();

	if ( mDrawDisplace )
	{
		gl::disableDepthRead();
		gl::disableDepthWrite();
		gl::setMatricesWindow( getWindowSize() );

		gl::color( Color::white() );
		const gl::Texture displaceMap = mHeart.getDisplacementTexture();
		gl::draw( displaceMap, Rectf( getWindowBounds() ) * .3f + Vec2f( getWindowWidth() * .7f, 0.f ) );
	}

	params::InterfaceGl::draw();
}

void HeartRateApp::keyDown( KeyEvent event )
{
	if ( event.getChar() == 'f' )
		setFullScreen( !isFullScreen() );
	if ( event.getCode() == KeyEvent::KEY_ESCAPE )
		quit();
}

void HeartRateApp::mouseDown( MouseEvent event )
{
	mArcball.mouseDown( event.getPos() );
}

void HeartRateApp::mouseDrag( MouseEvent event )
{
	mArcball.mouseDrag( event.getPos() );
}

void HeartRateApp::resize( ResizeEvent event )
{
	mCamera.setPerspective( 60.f, event.getAspectRatio(), 0.1f, 1000.0f );
}

CINDER_APP_BASIC( HeartRateApp, RendererGl( RendererGl::AA_NONE ) )


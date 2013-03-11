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
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Light.h"
#include "cinder/params/Params.h"

#include "cinder/Arcball.h"
#include "cinder/Camera.h"
#include "cinder/Cinder.h"

#include "mndlkit/params/PParams.h"

#include "HeartBloom.h"
#include "HeartShape.h"
#include "PulseSensorManager.h"

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

		void resize();

		void update();
		void draw();

	private:
		HeartShape mHeart;

		Arcball mArcball;
		CameraPersp mCamera;

		shared_ptr< gl::Light > mLight;

		static const int FBO_WIDTH;
		static const int FBO_HEIGHT;
		gl::Fbo mFbo;

		HeartBloom mHeartBloom;
		int mBloomIterations;
		float mBloomStrength;

		mndl::kit::params::PInterfaceGl mParams;

		HeartRate::PulseSensorManager mPulseSensorManager;
		void heartbeatCallback0( int data );
		void heartbeatCallback1( int data );

		// debug
		float mFps;
		bool mDrawDisplace;
		/*
		float mBpm0, mBpm1; // heart rate
		*/
		float mAmplitude0, mAmplitude1; // heart amplitude increased by inflation in every frame
		float mInflation0, mInflation1; // current inflation
		float mMaxInflation0, mMaxInflation1; // maximum inflation on heartbeat
		float mInflationDamping0, mInflationDamping1; // inflation damping per frame
		float mDamping0, mDamping1; // heart amplitude damping per frame

		void updateSignal();
};

const int HeartRateApp::FBO_WIDTH = 1024;
const int HeartRateApp::FBO_HEIGHT = 768;

void HeartRateApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1024, 768 );
}

void HeartRateApp::setup()
{
	gl::disableVerticalSync();

	// params
	mndl::kit::params::PInterfaceGl::load( "params.xml" );

	mParams = mndl::kit::params::PInterfaceGl( "Parameters", Vec2i( 300, 400 ) );
	mParams.addPersistentSizeAndPosition();

	// heart shape
	/*
	mParams.addPersistentParam( "Bpm left", &mBpm0, 60.f, "min=0 max=180 step=.1" );
	mParams.addPersistentParam( "Bpm right", &mBpm1, 60.f, "min=0 max=180 step=.1" );
	*/
	mAmplitude0 = 0.f;
	mInflation0 = 0.f;
	mParams.addPersistentParam( "Maximum Inflation left", &mMaxInflation0, .5f, "min=0.001 max=1.0 step=.001" );
	mParams.addPersistentParam( "Inflation damping left", &mInflationDamping0, .5f, "min=0 max=1 step=.001" );
	mParams.addPersistentParam( "Damping left", &mDamping0, .98f, "min=0 max=1 step=.001" );
	mAmplitude1 = 0.f;
	mInflation1 = 0.f;
	mParams.addPersistentParam( "Maximum Inflation right", &mMaxInflation1, .5f, "min=0.001 max=1.0 step=.001" );
	mParams.addPersistentParam( "Inflation damping right", &mInflationDamping1, .5f, "min=0 max=1 step=.001" );
	mParams.addPersistentParam( "Damping right", &mDamping1, .98f, "min=0 max=1 step=.001" );

	mParams.addSeparator();
	mParams.addPersistentParam( "Bloom iterations", &mBloomIterations, 8, "min=0 max=10" );
	mParams.addPersistentParam( "Bloom strength", &mBloomStrength, 0.4f, "min=0 max=5 step=0.005" );

	// debug
	mFps = 0;
	mParams.addParam( "Fps", &mFps, "", false );
	mParams.addSeparator();
	mParams.addPersistentParam( "Draw displace map", &mDrawDisplace, false );

	mHeart.setup();

	// heart fbo
	mFbo = gl::Fbo( FBO_WIDTH, FBO_HEIGHT );
	mHeartBloom = HeartBloom( FBO_WIDTH, FBO_HEIGHT );

    // set up the arcball
    mArcball = Arcball( getWindowSize() );
    mArcball.setRadius( getWindowWidth() * 0.5f );

	// set up the camera
	mCamera.setPerspective( 60.f, mFbo.getAspectRatio(), 0.1f, 1000.0f );
	mCamera.lookAt( Vec3f( 0.f, -30.f, -200.f ), Vec3f( 0.0f, -30.f, 0.0f ) );

	// light
	mLight = shared_ptr< gl::Light >( new gl::Light( gl::Light::DIRECTIONAL, 0 ) );
	mLight->setAmbient( ColorA::gray( .5 ) );
	mLight->setDiffuse( ColorA::gray( .5 ) );
	mLight->setDirection( Vec3f( -1, -1, -1 ) );
	//mLight->setPosition( Vec3f::one() * -1.0f );
	mLight->setSpecular( ColorA::white() );

	// pulsesensor
	mPulseSensorManager.setup();
	mPulseSensorManager.addCallback< HeartRateApp >( 0, HeartRate::PulseSensor::MT_BeatPauseTime,
														&HeartRateApp::heartbeatCallback0, this );
	mPulseSensorManager.addCallback< HeartRateApp >( 1, HeartRate::PulseSensor::MT_BeatPauseTime,
														&HeartRateApp::heartbeatCallback1, this );
}

void HeartRateApp::shutdown()
{
	mndl::kit::params::PInterfaceGl::save();
}

#if 0
void HeartRateApp::updateSignal()
{
	static double lastSeconds = 0.;
	static double phase0 = 0.;
	static double phase1 = 0.;

	double seconds = getElapsedSeconds();
	double delta = seconds - lastSeconds;
	lastSeconds = seconds;

	float s = M_PI * delta * getFrameRate() / 60.f;
	phase0 += s * mBpm0 / 60.f;
	phase1 += s * mBpm1 / 60.f;

	mAmplitude0 = math< float >::clamp( math< float >::sin( phase0 ) );
	mAmplitude1 = math< float >::clamp( math< float >::sin( phase1 ) );
	mHeart.setAmplitudes( mAmplitude0, mAmplitude1 );
}
#endif

void HeartRateApp::updateSignal()
{
	mAmplitude0 += mInflation0;
	mInflation0 *= mInflationDamping0;
	mAmplitude0 *= mDamping0;

	mAmplitude1 += mInflation1;
	mInflation1 *= mInflationDamping1;
	mAmplitude1 *= mDamping1;

	mHeart.setAmplitudes( mAmplitude0, mAmplitude1 );
}

void HeartRateApp::update()
{
	mLight->update( mCamera );

	updateSignal();
	mHeart.update();

	mFps = getAverageFps();
	mPulseSensorManager.update();
}

void HeartRateApp::heartbeatCallback0( int data )
{
	mInflation0 = mMaxInflation0;
}

void HeartRateApp::heartbeatCallback1( int data )
{
	mInflation1 = mMaxInflation1;
}

void HeartRateApp::draw()
{
	mFbo.bindFramebuffer();

	gl::clear( Color::black() );

	gl::setViewport( mFbo.getBounds() );
	gl::setMatrices( mCamera );
	gl::multModelView( mArcball.getQuat() );
	gl::multModelView( Matrix44f::createRotation( Vec3f( 0, M_PI, 0 ) ) );

	mLight->enable();
	mHeart.draw();
	mLight->disable();
	mFbo.unbindFramebuffer();

	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );

	gl::clear();
	gl::disableDepthRead();
	gl::disableDepthWrite();

	gl::color( Color::white() );

	gl::Texture heartOutput = mHeartBloom.process( mFbo.getTexture(), mBloomIterations,
			mBloomStrength * mAmplitude0, mBloomStrength * mAmplitude1 );
	heartOutput.setFlipped();
	Area outputArea = Area::proportionalFit( heartOutput.getBounds(), getWindowBounds(), true, true );
	gl::draw( heartOutput, outputArea );

	if ( mDrawDisplace )
	{
		Vec2f pos( getWindowWidth() * .8f, 0.f );
		Vec2f posStep( 0.f, getWindowHeight() * .2f );
		Vec2f labelOffset( getWindowWidth() * .005f, getWindowHeight() * .01f );

		gl::color( Color::white() );
		const gl::Texture displaceMap = mHeart.getDisplacementTexture();
		gl::draw( displaceMap, Rectf( getWindowBounds() ) * .2f + pos );
		gl::drawString( "displace", pos + labelOffset );

		pos += posStep;
		const gl::Texture normalMap = mHeart.getNormalTexture();
		gl::draw( normalMap, Rectf( getWindowBounds() ) * .2f + pos );
		gl::drawString( "normals", pos + labelOffset );

		pos += posStep;
		const gl::Texture bloomMap = mHeartBloom.getStrenghtTexture();
		gl::draw( bloomMap, Rectf( getWindowBounds() ) * .2f + pos );
		gl::drawString( "bloom", pos + labelOffset );
	}

	mndl::kit::params::PInterfaceGl::draw();
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

void HeartRateApp::resize()
{
}

CINDER_APP_BASIC( HeartRateApp, RendererGl( RendererGl::AA_NONE ) )


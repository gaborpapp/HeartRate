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

#include <algorithm>
#include <numeric>

#include <boost/format.hpp>

#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Light.h"
#include "cinder/params/Params.h"

#include "cinder/Camera.h"
#include "cinder/Cinder.h"
#include "cinder/Font.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"
#include "cinder/Text.h"
#include "cinder/Timeline.h"

#include "mndlkit/params/PParams.h"

#include "HeartBloom.h"
#include "HeartShape.h"
#include "PulseSensorManager.h"
#include "Resources.h"

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

		CameraPersp mCamera;

		static const int FBO_WIDTH;
		static const int FBO_HEIGHT;
		gl::Fbo mFbo;
		gl::Fbo mStatisticsFbo;

		HeartBloom mHeartBloom;
		int mBloomIterations;
		float mBloomStrength;

		mndl::params::PInterfaceGl mParams;

		HeartRate::PulseSensorManager mPulseSensorManager;
		void heartbeatCallback0( int data );
		void heartbeatCallback1( int data );

		// debug
		float mFps;
		bool mVerticalSyncEnabled;
		bool mDrawDisplace;
		/*
		float mBpm0, mBpm1; // heart rate
		*/
		float mAmplitude0, mAmplitude1; // heart amplitude increased by inflation in every frame
		float mInflation0, mInflation1; // current inflation
		float mMaxInflation0, mMaxInflation1; // maximum inflation on heartbeat
		float mInflationDamping0, mInflationDamping1; // inflation damping per frame
		float mDamping0, mDamping1; // heart amplitude damping per frame

		void pulseCallback0( int data );
		void pulseCallback1( int data );
		int mPulse0, mPulse1;
		int mInitialPulse0, mInitialPulse1;
		int mLastPulse0, mLastPulse1;

		void updateSignal();
		void updateStatistics();
		void renderStatistics();

		void drawHeart();
		void drawInfo();

		float mFontSizeBig, mFontSizeMiddle, mFontSizeSmall;
		Color mTextColor0, mTextColor1, mTextColor2;

		Font mFontBig, mFontMiddle, mFontSmall, mFontSmaller;

		void initSetup();
		void initGame();
		void startGame();

		gl::GlslProg mPulseShader;

		int mHarmony;
		int mHarmony0, mHarmony1;
		int mMinHarmony0, mMaxHarmony0;
		int mMinHarmony1, mMaxHarmony1;
		vector< int > mPulses0, mPulses1;
		vector< int > mHarmonies;

		gl::Texture mRulesTxt;
		gl::Texture mStatisticsTxt;
		Anim< float > mFade;
		Anim< int > mCountdown;
		int mCountdownDuration;
		int mHarmonyIgnoreDuration;
		int mHarmonyStabilizer;

		enum
		{
			STATE_RULES = 0,
			STATE_SETUP,
			STATE_GAME,
			STATE_STATISTICS
		};
		int mState;
		double mGameStartTime;
};

const int HeartRateApp::FBO_WIDTH = 1280;
const int HeartRateApp::FBO_HEIGHT = 720;

void HeartRateApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1280, 720 );
}

void HeartRateApp::setup()
{
	gl::disableVerticalSync();

	// params
	mndl::params::PInterfaceGl::load( "params.xml" );

	mParams = mndl::params::PInterfaceGl( "Parameters", Vec2i( 300, 400 ) );
	mParams.addPersistentSizeAndPosition();

	// heart shape
	/*
	mParams.addPersistentParam( "Bpm left", &mBpm0, 60.f, "min=0 max=180 step=.1" );
	mParams.addPersistentParam( "Bpm right", &mBpm1, 60.f, "min=0 max=180 step=.1" );
	*/
	mParams.addPersistentParam( "Maximum Inflation left", &mMaxInflation0, .5f, "min=0.001 max=1.0 step=.001" );
	mParams.addPersistentParam( "Inflation damping left", &mInflationDamping0, .5f, "min=0 max=1 step=.001" );
	mParams.addPersistentParam( "Damping left", &mDamping0, .98f, "min=0 max=1 step=.001" );
	mParams.addPersistentParam( "Maximum Inflation right", &mMaxInflation1, .5f, "min=0.001 max=1.0 step=.001" );
	mParams.addPersistentParam( "Inflation damping right", &mInflationDamping1, .5f, "min=0 max=1 step=.001" );
	mParams.addPersistentParam( "Damping right", &mDamping1, .98f, "min=0 max=1 step=.001" );

	mParams.addSeparator();
	mParams.addPersistentParam( "Bloom iterations", &mBloomIterations, 8, "min=0 max=10" );
	mParams.addPersistentParam( "Bloom strength", &mBloomStrength, 0.3f, "min=0 max=5 step=0.005" );

	// debug
	mFps = 0;
	mParams.addParam( "Fps", &mFps, "", false );
	mParams.addPersistentParam( "Vertical sync", &mVerticalSyncEnabled, true );
	mParams.addSeparator();
	mParams.addPersistentParam( "Draw displace map", &mDrawDisplace, false );

	mParams.addSeparator();
	mParams.addPersistentParam( "Font size big", &mFontSizeBig, 54, "min=1 step=.5" );
	mParams.addPersistentParam( "Font size middle", &mFontSizeMiddle, 48, "min=1 step=.5" );
	mParams.addPersistentParam( "Font size small", &mFontSizeSmall, 26, "min=1 step=.5" );
	mParams.addPersistentParam( "Text color 0", &mTextColor0, Color( 1.f, 0.f, 0.f ) );
	mParams.addPersistentParam( "Text color 1", &mTextColor1, Color::gray( .5 ) );
	mParams.addPersistentParam( "Text color 2", &mTextColor2, Color( 0.f, 1.f, 0.f ) );
	mParams.addSeparator();
	mParams.addPersistentParam( "Countdown duration", &mCountdownDuration, 5 * 60, "min=10" );
	mParams.addPersistentParam( "Harmony ignore duration", &mHarmonyIgnoreDuration, 30, "min=0" );
	mParams.addPersistentParam( "Harmony stabilizer", &mHarmonyStabilizer, 20, "min=0" );

	mHeart.setup();

	// heart fbo
	gl::Fbo::Format format;
	format.setSamples( 4 );
	mFbo = gl::Fbo( FBO_WIDTH, FBO_HEIGHT, format );
	mStatisticsFbo = gl::Fbo( FBO_WIDTH, FBO_HEIGHT, format );
	mHeartBloom = HeartBloom( FBO_WIDTH, FBO_HEIGHT );

	// set up the camera
	mCamera.setPerspective( 60.f, mFbo.getAspectRatio(), 0.1f, 1000.0f );
	mCamera.lookAt( Vec3f( 0.f, -30.f, -200.f ), Vec3f( 0.0f, -30.f, 0.0f ) );

	// pulsesensor
	mPulseSensorManager.setup();
	mPulseSensorManager.addCallback< HeartRateApp >( 0, HeartRate::PulseSensor::MT_BeatPauseTime,
														&HeartRateApp::heartbeatCallback0, this );
	mPulseSensorManager.addCallback< HeartRateApp >( 1, HeartRate::PulseSensor::MT_BeatPauseTime,
														&HeartRateApp::heartbeatCallback1, this );

	mPulseSensorManager.addCallback< HeartRateApp >( 0, HeartRate::PulseSensor::MT_BeatPerMinute,
														&HeartRateApp::pulseCallback0, this );
	mPulseSensorManager.addCallback< HeartRateApp >( 1, HeartRate::PulseSensor::MT_BeatPerMinute,
														&HeartRateApp::pulseCallback1, this );
	try
	{
		mPulseShader = gl::GlslProg( app::loadResource( RES_PULSE_VERT ),
									 app::loadResource( RES_PULSE_FRAG ) );
		mPulseShader.bind();
		mPulseShader.uniform( "txt", 0 );
		mPulseShader.unbind();
	}
	catch ( const gl::GlslProgCompileExc &exc )
	{
		app::console() << exc.what() << endl;
	}
	catch ( const app::ResourceLoadExc &exc )
	{
		app::console() << exc.what() << endl;
	}

	mFontBig = Font( app::loadResource( RES_FONT ), mFontSizeBig );
	mFontMiddle = Font( app::loadResource( RES_FONT ), mFontSizeMiddle );
	mFontSmall = Font( app::loadResource( RES_FONT ), mFontSizeSmall );
	mFontSmaller = Font( app::loadResource( RES_FONT ), mFontSizeSmall * .5f );

	try
	{
		mRulesTxt = loadImage( loadAsset( "gfx/rules.png" ) );
	}
	catch ( const app::AssetLoadExc &exc )
	{
		app::console() << exc.what() << endl;
	}

	mParams.addSeparator();
	mParams.addButton( "Start game", std::bind( &HeartRateApp::startGame, this ) );

	mState = STATE_RULES;
	mFade = 1.f;
	initSetup();
	initGame();
	updateSignal();
	mHeart.update( mCamera );
}

void HeartRateApp::shutdown()
{
	mndl::params::PInterfaceGl::save();
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

	if ( mState == STATE_GAME )
		mHeart.setAmplitudes( mAmplitude0, mAmplitude1 );
	else
	if ( ( mState == STATE_RULES ) || ( mState == STATE_SETUP ) )
		mHeart.setAmplitudes( 0.f, 0.f );
}

void HeartRateApp::updateStatistics()
{
	static int lastHarmony = -1;
	static int lastHarmonyStableFrames = 0;

	if ( ( mAmplitude0 == 0.f ) || ( mAmplitude1 == 0.f ) )
	{
		mHarmony = -1;
	}
	else
	{
		int harmony = int( 100.f * math< float >::min( mAmplitude0, mAmplitude1 ) /
						math< float >::max( mAmplitude0, mAmplitude1 ) );
		if ( lastHarmony == harmony )
		{
			lastHarmonyStableFrames++;
			if ( lastHarmonyStableFrames >= mHarmonyStabilizer )
			{
				mHarmony = harmony;
				if ( ( getElapsedSeconds() - mGameStartTime ) >= mHarmonyIgnoreDuration )
				{
					mHarmonies.push_back( mHarmony );
				}
			}
		}
		else
		{
			lastHarmonyStableFrames = 0;
		}
		lastHarmony = harmony;
	}

	mHarmony0 = int( 100.f * mAmplitude0 );
	mHarmony1 = int( 100.f * mAmplitude1 );
	mMinHarmony0 = math< int >::min( mMinHarmony0, mHarmony0 );
	mMaxHarmony0 = math< int >::max( mMaxHarmony0, mHarmony0 );
	mMinHarmony1 = math< int >::min( mMinHarmony1, mHarmony1 );
	mMaxHarmony1 = math< int >::max( mMaxHarmony1, mHarmony1 );
}

void HeartRateApp::renderStatistics()
{
	string minPulse0Str, maxPulse0Str, meanPulse0Str;
	string minPulse1Str, maxPulse1Str, meanPulse1Str;
	string minPulse0DeltaStr, maxPulse0DeltaStr, meanPulse0DeltaStr;
	string minPulse1DeltaStr, maxPulse1DeltaStr, meanPulse1DeltaStr;
	int minPulse, maxPulse;

	if ( mPulses0.empty() )
	{
		minPulse0Str = " -- ";
		maxPulse0Str = " -- ";
		meanPulse0Str = " -- ";
		minPulse0DeltaStr = " -- ";
		maxPulse0DeltaStr = " -- ";
		meanPulse0DeltaStr = " -- ";
	}
	else
	{
		auto bounds0 = std::minmax_element( mPulses0.begin(), mPulses0.end() );
		minPulse = *bounds0.first;
		minPulse0Str = ( boost::format( "%4d" ) % ( *bounds0.first ) ).str();
		minPulse0DeltaStr = ( boost::format( "%+4d" ) % ( *bounds0.first - mInitialPulse0 ) ).str();
		maxPulse = *bounds0.second;
		maxPulse0Str = ( boost::format( "%4d" ) % ( *bounds0.second ) ).str();
		maxPulse0DeltaStr = ( boost::format( "%+4d" ) % ( *bounds0.second - mInitialPulse0 ) ).str();
		long meanPulse0 = std::accumulate( mPulses0.begin(), mPulses0.end(), 0L ) / mPulses0.size();
		meanPulse0Str = ( boost::format( "%4d" ) % ( meanPulse0 ) ).str();
		meanPulse0DeltaStr = ( boost::format( "%+4d" ) % ( meanPulse0 - mInitialPulse0 ) ).str();
	}

	if ( mPulses1.empty() )
	{
		minPulse1Str = " -- ";
		maxPulse1Str = " -- ";
		meanPulse1Str = " -- ";
		minPulse1DeltaStr = " -- ";
		maxPulse1DeltaStr = " -- ";
		meanPulse1DeltaStr = " -- ";
	}
	else
	{
		auto bounds1 = std::minmax_element( mPulses1.begin(), mPulses1.end() );
		minPulse = math< int >::min( minPulse, *bounds1.first );
		minPulse1Str = ( boost::format( "%4d" ) % ( *bounds1.first ) ).str();
		minPulse1DeltaStr = ( boost::format( "%+4d" ) % ( *bounds1.first - mInitialPulse1 ) ).str();
		maxPulse = math< int >::max( maxPulse, *bounds1.second );
		maxPulse1Str = ( boost::format( "%4d" ) % ( *bounds1.second ) ).str();
		maxPulse1DeltaStr = ( boost::format( "%+4d" ) % ( *bounds1.second - mInitialPulse1 ) ).str();
		long meanPulse1 = std::accumulate( mPulses1.begin(), mPulses1.end(), 0L ) / mPulses1.size();
		meanPulse1Str = ( boost::format( "%4d" ) % ( meanPulse1 ) ).str();
		meanPulse1DeltaStr = ( boost::format( "%+4d" ) % ( meanPulse1 - mInitialPulse1 ) ).str();
	}

	TextLayout layout;
	layout.clear( ColorA::gray( 0.f, 0.8f ) );
	layout.setBorder( 10, 10 );

	auto addLine = [ & ]( string s0, string s0delta, string s1, string s2, string s2delta )
	{
		layout.setFont( mFontSmall );
		layout.setColor( mTextColor1 );
		layout.addCenteredLine( s0delta );
		layout.setFont( mFontMiddle );
		layout.setColor( mTextColor0 );
		layout.append( s0 );
		layout.setFont( mFontSmall );
		layout.setColor( mTextColor1 );
		layout.append( s1 );
		layout.setFont( mFontMiddle );
		layout.setColor( mTextColor0 );
		layout.append( s2 );
		layout.setFont( mFontSmall );
		layout.setColor( mTextColor1 );
		layout.append( s2delta );
	};

	addLine( minPulse0Str, minPulse0DeltaStr + " ", "  minimum pulse ", minPulse1Str, " " + minPulse1DeltaStr );
	addLine( maxPulse0Str, maxPulse0DeltaStr + " ", "  maximum pulse ", maxPulse1Str, " " + maxPulse1DeltaStr );
	addLine( meanPulse0Str, meanPulse0DeltaStr + " ", "  average pulse ", meanPulse1Str, " " + meanPulse1DeltaStr );

	string minHarmonyStr, maxHarmonyStr, meanHarmonyStr;
	string minHarmony0DeltaStr, maxHarmony0DeltaStr;
	string minHarmony1DeltaStr, maxHarmony1DeltaStr;
	string meanHarmonyDeltaStr;
	long meanHarmony = -1;

	if ( mHarmonies.empty() )
	{
		minHarmonyStr = " -- ";
		maxHarmonyStr = " -- ";
		meanHarmonyStr = " -- ";
		minHarmony0DeltaStr = " -- ";
		maxHarmony0DeltaStr = " -- ";
		minHarmony1DeltaStr = " -- ";
		maxHarmony1DeltaStr = " -- ";
		meanHarmonyDeltaStr = " -- ";
	}
	else
	{
		int initialHarmony = mHarmonies[ 0 ];
		auto bounds = std::minmax_element( mHarmonies.begin(), mHarmonies.end() );
		minHarmonyStr = ( boost::format( "%3d%%" ) % ( *bounds.first ) ).str();
		minHarmony0DeltaStr = ( boost::format( "%+4d" ) % mMinHarmony0 ).str();
		minHarmony1DeltaStr = ( boost::format( "%+4d" ) % mMinHarmony1 ).str();
		maxHarmonyStr = ( boost::format( "%3d%%" ) % ( *bounds.second ) ).str();
		maxHarmony0DeltaStr = ( boost::format( "%+4d" ) % mMaxHarmony0 ).str();
		maxHarmony1DeltaStr = ( boost::format( "%+4d" ) % mMaxHarmony1 ).str();
		meanHarmony = std::accumulate( mHarmonies.begin(), mHarmonies.end(), 0L ) / mHarmonies.size();
		meanHarmonyStr = ( boost::format( "%3d%%" ) % ( meanHarmony ) ).str();
		meanHarmonyDeltaStr = ( boost::format( "%+4d" ) % ( meanHarmony - initialHarmony ) ).str();
	}

	addLine( minHarmonyStr, minHarmony0DeltaStr + " ", " minimum harmony", minHarmonyStr, " " + minHarmony1DeltaStr );
	addLine( maxHarmonyStr, maxHarmony0DeltaStr + " ", " maximum harmony", maxHarmonyStr, " " + maxHarmony1DeltaStr );
	addLine( meanHarmonyStr, meanHarmonyDeltaStr + " ", " average harmony", meanHarmonyStr, " " + meanHarmonyDeltaStr );

	Surface8u statisticFigures = layout.render( true );

	gl::SaveFramebufferBinding bindingSaver;
	glPushAttrib( GL_VIEWPORT_BIT );
	gl::pushMatrices();

	mStatisticsFbo.bindFramebuffer();

	gl::setViewport( mStatisticsFbo.getBounds() );
	gl::setMatricesWindow( mStatisticsFbo.getSize(), false );
	gl::clear( ColorA::gray( 0.f, 0.8f ) );

	// numbers
	gl::draw( gl::Texture( statisticFigures ),
			  Vec2f( 0, ( mFbo.getHeight() - statisticFigures.getHeight() ) / 2 ) );

	// pulse diagram
	Rectf diagramRect = Rectf( mStatisticsFbo.getBounds() ).scaled( .5f ) +
								Vec2f( mStatisticsFbo.getWidth() / 2, 0.f );
	diagramRect.scaleCentered( .7f );

	if ( mPulses0.size() > 1 )
	{
		RectMapping mapping( Rectf( 0, minPulse - 50, mPulses0.size() - 1, maxPulse + 50 ),
				diagramRect );
		glLineWidth( 2.f );
		gl::begin( GL_LINE_STRIP );
		gl::color( Color( 0, 0, 1 ) );
		for ( size_t i = 0; i < mPulses0.size(); i++ )
		{
			gl::vertex( mapping.map( Vec2f( i, mPulses0[ i ] ) ) );
		}
		gl::end();
		gl::color( Color::white() );
		glLineWidth( 1.f );

		// draw left
		TextBox diagramLeft;
		diagramLeft.font( mFontSmall ).alignment( TextBox::CENTER ).color( Color( 0, 0, 1 ) ).size( 100, TextBox::GROW );
		diagramLeft.backgroundColor( ColorA::gray( 0.f, 0.8f ) );
		diagramLeft.setText( "left" );
		Vec2f diagramLeftPos = diagramRect.getUpperLeft() + Vec2f( diagramRect.getWidth() / 4 - diagramLeft.measure().x / 2, -mFontSmall.getSize() * 1.5 );
		gl::draw( diagramLeft.render(), diagramLeftPos );
	}

	if ( mPulses1.size() > 1 )
	{
		RectMapping mapping( Rectf( 0, minPulse - 50, mPulses1.size() - 1, maxPulse + 50 ),
				diagramRect );
		glLineWidth( 2.f );
		gl::begin( GL_LINE_STRIP );
		gl::color( Color( 1, 0, 0 ) );
		for ( size_t i = 0; i < mPulses1.size(); i++ )
		{
			gl::vertex( mapping.map( Vec2f( i, mPulses1[ i ] ) ) );
		}
		gl::end();
		gl::color( Color::white() );
		glLineWidth( 1.f );

		// draw right
		TextBox diagramRight;
		diagramRight.font( mFontSmall ).alignment( TextBox::CENTER ).color( Color( 1, 0, 0 ) ).size( 100, TextBox::GROW );
		diagramRight.backgroundColor( ColorA::gray( 0.f, 0.8f ) );
		diagramRight.setText( "right" );
		Vec2f diagramRightPos = diagramRect.getUpperRight() + Vec2f( -diagramRect.getWidth() / 4 - diagramRight.measure().x / 2, -mFontSmall.getSize() * 1.5 );
		gl::draw( diagramRight.render(), diagramRightPos );
	}

	if ( ( mPulses0.size() > 1 ) || ( mPulses1.size() > 1 ) )
	{
		gl::color( Color::white() );
		gl::drawStrokedRect( diagramRect );

		// draw time
		Vec2f diagramTimePos;
		float diagramTimePosStep = diagramRect.getWidth() / 5;
		for( int i = 0; i <= 5; ++i )
		{
			TextBox diagramTime;

			diagramTime.font( mFontSmaller ).alignment( TextBox::LEFT ).color( mTextColor1 ).size( 150, TextBox::GROW );
			diagramTime.backgroundColor( ColorA::gray( 0.f, 0.8f ) );
			diagramTime.setText( toString( i ) + ":00" );
			if( i == 0 )
				diagramTimePos = diagramRect.getLowerLeft() + Vec2f( -diagramTime.measure().x / 2, mFontSmaller.getSize() );
			gl::draw( diagramTime.render(), diagramTimePos );
			diagramTimePos.x += diagramTimePosStep;
		}
	}

	// harmony heart
	if ( meanHarmony != -1 )
	{
		float a0 = Rand::randFloat( 0.5f, 1.f );
		float a1 = a0 / ( meanHarmony * .01f );
		mHeart.setAmplitudes( a1, a0 );
		mHeart.update( mCamera );

		Rectf heartRect = Rectf( mStatisticsFbo.getBounds() ).scaled( Vec2f( 0.5f, 1.f ) ) +
							Vec2f( mStatisticsFbo.getWidth() / 2, 0.f );
		gl::setViewport( Area( heartRect ) );
		CameraPersp cam( mCamera );
		cam.setAspectRatio( heartRect.getAspectRatio() );
		cam.lookAt( Vec3f( 0.f, -60.f, -400.f ), Vec3f( 0.0f, -60.f, 0.0f ) );
		gl::setMatrices( cam );
		gl::multModelView( Matrix44f::createRotation( Vec3f( 0, M_PI, M_PI ) ) );

		mHeart.draw();
	}

	gl::popMatrices();
	glPopAttrib();
	mStatisticsFbo.unbindFramebuffer();

	mStatisticsTxt = mStatisticsFbo.getTexture();
}

void HeartRateApp::update()
{
	mFps = getAverageFps();
	if ( mVerticalSyncEnabled != gl::isVerticalSyncEnabled() )
		gl::enableVerticalSync( mVerticalSyncEnabled );

	if ( ( mState == STATE_GAME ) || ( mState == STATE_SETUP ) )
	{
		updateSignal();
		mHeart.update( mCamera );
		mPulseSensorManager.update();
	}

	if ( mState == STATE_GAME )
		updateStatistics();
}

void HeartRateApp::heartbeatCallback0( int data )
{
	if ( ( mState != STATE_GAME ) && ( mState != STATE_SETUP ) )
		return;

	mInflation0 = mMaxInflation0;
}

void HeartRateApp::heartbeatCallback1( int data )
{
	if ( ( mState != STATE_GAME ) && ( mState != STATE_SETUP ) )
		return;

	mInflation1 = mMaxInflation1;
}

void HeartRateApp::pulseCallback0( int data )
{
	if ( ( mState != STATE_GAME ) && ( mState != STATE_SETUP ) )
		return;

	data = math< int >::clamp( data, 40, 200 );
	if ( ( mInitialPulse0 == 0 ) && ( mState == STATE_GAME ) )
		mInitialPulse0 = data;
	mLastPulse0 = mPulse0;
	mPulse0 = data;
	if ( mState == STATE_GAME )
		mPulses0.push_back( mPulse0 );
}

void HeartRateApp::pulseCallback1( int data )
{
	if ( ( mState != STATE_GAME ) && ( mState != STATE_SETUP ) )
		return;

	data = math< int >::clamp( data, 40, 200 );
	if ( ( mInitialPulse1 == 0 ) && ( mState == STATE_GAME ) )
		mInitialPulse1 = data;
	mLastPulse1 = mPulse1;
	mPulse1 = data;
	if ( mState == STATE_GAME )
		mPulses1.push_back( mPulse1 );
}

void HeartRateApp::initSetup()
{
	mHarmony = -1;
	mPulse0 = mPulse1 = 0;
	mAmplitude0 = mAmplitude1 = 0.f;
	mInflation0 = mInflation1 = 0.f;
	mInitialPulse0 = mInitialPulse1 = 0;
}

void HeartRateApp::initGame()
{
	mInitialPulse0 = mPulse0;
	mInitialPulse1 = mPulse1;
	mAmplitude0 = mAmplitude1 = 0.f;
	mMinHarmony0 = mMinHarmony1 = 100;
	mMaxHarmony0 = mMaxHarmony1 = 0;
	mInflation0 = mInflation1 = 0.f;
	mPulses0.clear();
	mPulses1.clear();
	mHarmonies.clear();
	mLastPulse0 = mLastPulse1 = 0;
	mStatisticsTxt.reset();
}

void HeartRateApp::startGame()
{
	if ( ( mState == STATE_RULES ) || ( mState == STATE_STATISTICS ) )
	{
		initSetup();
		mFade = 1.f;
		timeline().apply( &mFade, 0.f, 1.f ).finishFn( [ & ]()
				{
					mState = STATE_SETUP;
					mCountdown = mCountdownDuration * 100;
				} );
	}
	else
	if ( mState == STATE_SETUP )
	{
		initGame();
		mState = STATE_GAME;
		mGameStartTime = getElapsedSeconds();
		mCountdown = mCountdownDuration * 100;
		timeline().apply( &mCountdown, 0, mCountdownDuration ).finishFn( [ & ]() { mState = STATE_STATISTICS; } );
	}
}

void HeartRateApp::drawInfo()
{
	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );

	gl::disableDepthRead();
	gl::disableDepthWrite();

	gl::enableAlphaBlending();

	gl::color( Color::white() );
	Vec2f winSize( getWindowSize() );

	// countdown
	Vec2f timerPos( winSize * Vec2f( .5f, .05f ) );
	TextBox timerBox;
	timerBox.font( mFontSmall ).alignment( TextBox::CENTER ).color( mTextColor1 ).size( 200, TextBox::GROW );
	boost::format timeFormater = boost::format( "%d:%02d:%02d" ) % ( mCountdown / ( 60 * 100 ) ) % ( ( mCountdown / 100 ) % 60 )
			% ( mCountdown % 100 );
	timerBox.setText( timeFormater.str() );
	gl::draw( timerBox.render(), timerPos - Vec2f( timerBox.getSize().x, timerBox.measure().y ) * .5f );

	// pulse textures
	gl::Texture pulseTxt0 = mPulseSensorManager.getSensor( 0 ).getPulseTexture();
	Rectf pulse0Rect( winSize * Vec2f( .05f, .8f ), winSize * Vec2f( .3f, .95f ) );
	if ( mPulseShader )
		mPulseShader.bind();
	gl::draw( pulseTxt0, pulse0Rect );
	if ( mPulseShader )
		mPulseShader.unbind();

	gl::Texture pulseTxt1 = mPulseSensorManager.getSensor( 1 ).getPulseTexture();
	Rectf pulse1Rect( Vec2f( winSize.x - pulse0Rect.getX2(), pulse0Rect.getY1() ),
					  Vec2f( winSize.x - pulse0Rect.getX1(), pulse0Rect.getY2() ) );
	// flip horizontally
	pulse1Rect.set( pulse1Rect.getX2(), pulse1Rect.getY1(),
					pulse1Rect.getX1(), pulse1Rect.getY2() );
	if ( mPulseShader )
		mPulseShader.bind();
	gl::draw( pulseTxt1, pulse1Rect );
	if ( mPulseShader )
		mPulseShader.unbind();

	// pulse values
	auto mirror = [ &winSize ]( Vec2f p ) { return Vec2f( winSize.x - p.x, p.y ); };
	Vec2f pulse0Pos( winSize * Vec2f( .11f, .74f ) );
	Vec2f pulse1Pos = mirror( pulse0Pos );
	TextBox pulseBox;
	// FIXME: does not work with size TextBox::GROW, TextBox::GROW
	pulseBox.font( mFontBig ).alignment( TextBox::CENTER ).color( mTextColor0 ).size( 200, TextBox::GROW );

	string pulse0Str, pulse1Str;
	if ( mPulse0 )
		pulse0Str = toString( mPulse0 );
	else
		pulse0Str = "--";
	if ( mPulse1 )
		pulse1Str = toString( mPulse1 );
	else
		pulse1Str = "--";
	pulseBox.setText( pulse0Str );
	gl::draw( pulseBox.render(), pulse0Pos - pulseBox.getSize() * .5f );
	pulseBox.setText( pulse1Str );
	gl::draw( pulseBox.render(), pulse1Pos - pulseBox.getSize() * .5f );

	// pulse delta
	Vec2f pulse0DeltaPos = pulse0Pos - Vec2f( 0, .6f * pulseBox.measure().y );
	Vec2f pulse1DeltaPos = mirror( pulse0DeltaPos );
	int pulse0Delta = mPulse0 - mInitialPulse0;
	int pulse1Delta = mPulse1 - mInitialPulse1;
	string pulse0DeltaStr, pulse1DeltaStr;
	if ( mPulse0 && ( mState == STATE_GAME ) )
	{
		pulse0DeltaStr = toString( pulse0Delta );
		if ( pulse0Delta > 0 )
			pulse0DeltaStr = "+" + pulse0DeltaStr;
	}
	else
		pulse0DeltaStr = "--";
	if ( mPulse1 && ( mState == STATE_GAME ) )
	{
		pulse1DeltaStr = toString( pulse1Delta );
		if ( pulse1Delta > 0 )
			pulse1DeltaStr = "+" + pulse1DeltaStr;
	}
	else
		pulse1DeltaStr = "--";
	TextBox pulseDeltaBox;
	TextBox arrowBox;
	const string arrowDown = "\xe2\x96\xbc";
	const string arrowUp = "\xe2\x96\xb2";

	pulseDeltaBox.font( mFontSmall ).alignment( TextBox::CENTER ).color( mTextColor1 ).size( 150, TextBox::GROW );
	pulseDeltaBox.setText( pulse0DeltaStr );
	gl::draw( pulseDeltaBox.render(), pulse0DeltaPos - pulseDeltaBox.getSize() * .5f );
	arrowBox.font( mFontSmall ).alignment( TextBox::CENTER ).size( 50, TextBox::GROW );
	int pulse0ArrowDelta = mPulse0 - mLastPulse0;
	if ( pulse0ArrowDelta > 0 )
	{
		arrowBox.setColor( mTextColor2 );
		arrowBox.setText( arrowUp );
	}
	else if ( pulse0ArrowDelta < 0 )
	{
		arrowBox.setColor( mTextColor0 );
		arrowBox.setText( arrowDown );
	}
	else
	{
		arrowBox.setColor( mTextColor1 );
		arrowBox.setText( " " );
	}
	gl::draw( arrowBox.render(), pulse0DeltaPos + Vec2f( .5f * pulseDeltaBox.measure().x, 0.f ) );

	pulseDeltaBox.setText( pulse1DeltaStr );
	gl::draw( pulseDeltaBox.render(), pulse1DeltaPos - pulseDeltaBox.getSize() * .5f );
	int pulse1ArrowDelta = mPulse1 - mLastPulse1;
	if ( pulse1ArrowDelta > 0 )
	{
		arrowBox.setColor( mTextColor2 );
		arrowBox.setText( arrowUp );
	}
	else if ( pulse1ArrowDelta < 0 )
	{
		arrowBox.setColor( mTextColor0 );
		arrowBox.setText( arrowDown );
	}
	else
	{
		arrowBox.setColor( mTextColor1 );
		arrowBox.setText( " " );
	}
	gl::draw( arrowBox.render(), pulse1DeltaPos + Vec2f( .5f * pulseDeltaBox.measure().x, 0.f ) );

	// harmony
	Vec2f harmonyPos( winSize * Vec2f( .5f, .95f ) );
	TextBox harmonyBox;
	harmonyBox.font( mFontMiddle ).alignment( TextBox::CENTER ).color( mTextColor0 ).size( 150, TextBox::GROW );
	string harmonyStr;
	if ( ( mHarmony >= 0 ) && ( mState == STATE_GAME ) )
		harmonyStr = toString( mHarmony ) + "%";
	else
		harmonyStr = "--";
	harmonyBox.setText( harmonyStr );
	gl::draw( harmonyBox.render(), harmonyPos - Vec2f( harmonyBox.getSize().x, harmonyBox.measure().y ) * .5f );

	TextBox harmonyDeltaBox;
	harmonyDeltaBox.font( mFontSmall ).alignment( TextBox::RIGHT ).color( mTextColor1 ).size( 90, TextBox::GROW );
	string harmonyDelta0Str;
	if ( mState == STATE_SETUP )
		harmonyDelta0Str = "--";
	else
	if ( mHarmony0 > 0 )
		harmonyDelta0Str = "+" + toString( mHarmony0 );
	else
		harmonyDelta0Str = toString( mHarmony0 );
	harmonyDeltaBox.setText( harmonyDelta0Str );
	Vec2f harmonyDeltaPos0 = harmonyPos - Vec2f( harmonyBox.measure().x, 0.f ) * .5f - winSize * Vec2f( .05f, 0.f );
	gl::draw( harmonyDeltaBox.render(),
			  harmonyDeltaPos0 -
				Vec2f( harmonyDeltaBox.getSize().x, harmonyDeltaBox.measure().y ) * .5f );
	string harmonyDelta1Str;
	if ( mState == STATE_SETUP )
		harmonyDelta1Str = "--";
	else
	if ( mHarmony1 > 0 )
		harmonyDelta1Str = "+" + toString( mHarmony1 );
	else
		harmonyDelta1Str = toString( mHarmony1 );
	harmonyDeltaBox.setText( harmonyDelta1Str );
	harmonyDeltaBox.setAlignment( TextBox::LEFT );
	Vec2f harmonyDeltaPos1 = mirror( harmonyDeltaPos0 );
	gl::draw( harmonyDeltaBox.render(),
				harmonyDeltaPos1 -
				   Vec2f( harmonyDeltaBox.getSize().x, harmonyDeltaBox.measure().y ) * .5f );

	gl::disableAlphaBlending();
}

void HeartRateApp::drawHeart()
{
	mFbo.bindFramebuffer();

	gl::clear( Color::black() );

	gl::setViewport( mFbo.getBounds() );
	gl::setMatrices( mCamera );
	gl::multModelView( Matrix44f::createRotation( Vec3f( 0, M_PI, 0 ) ) );

	mHeart.draw();
	mFbo.unbindFramebuffer();

	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );

	gl::disableDepthRead();
	gl::disableDepthWrite();

	gl::color( Color::white() );

	gl::Texture heartOutput = mHeartBloom.process( mFbo.getTexture(), mBloomIterations,
			mBloomStrength * mAmplitude0, mBloomStrength * mAmplitude1 );
	heartOutput.setFlipped();
	Rectf outputRect = Rectf( heartOutput.getBounds() ).getCenteredFit( getWindowBounds(), true );
	if ( outputRect.getWidth() < getWindowWidth() )
		outputRect.scaleCentered( float( getWindowWidth() ) / outputRect.getWidth() );
	else
	if ( outputRect.getHeight() < getWindowHeight() )
		outputRect.scaleCentered( float( getWindowHeight() ) / outputRect.getHeight() );

	gl::draw( heartOutput, outputRect );
}

void HeartRateApp::draw()
{
	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );

	gl::clear();
	gl::disableDepthRead();
	gl::disableDepthWrite();

	drawHeart();

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
		const gl::Texture bloomMap = mHeartBloom.getStrenghtTexture();
		gl::draw( bloomMap, Rectf( getWindowBounds() ) * .2f + pos );
		gl::drawString( "bloom", pos + labelOffset );
	}

	if ( ( mState == STATE_RULES ) && mRulesTxt )
	{
		Rectf outputRect = Rectf( mRulesTxt.getBounds() ).getCenteredFit( getWindowBounds(), true );
		gl::enableAlphaBlending();
		gl::color( ColorA::gray( 1.f, mFade ) );
		gl::draw( mRulesTxt, outputRect );
		gl::disableAlphaBlending();
	}

	if ( mState != STATE_RULES )
		drawInfo();

	if ( mState == STATE_STATISTICS )
	{
		if ( !mStatisticsTxt )
		{
			renderStatistics();
			mFade = 0.f;
			timeline().apply( &mFade, 1.f, 1.f );
		}
		gl::enableAlphaBlending();
		gl::color( ColorA::gray( 1.f, mFade ) );
		gl::draw( mStatisticsTxt, getWindowBounds() );
		gl::disableAlphaBlending();
	}

	mPulseSensorManager.draw();

	mParams.draw();
}

void HeartRateApp::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_f:
			if ( !isFullScreen() )
			{
				setFullScreen( true );
				if ( mParams.isVisible() )
					showCursor();
				else
					hideCursor();
			}
			else
			{
				setFullScreen( false );
				showCursor();
			}
			break;

		case KeyEvent::KEY_s:
			mndl::params::PInterfaceGl::showAllParams( !mParams.isVisible() );
			if ( isFullScreen() )
			{
				if ( mParams.isVisible() )
					showCursor();
				else
					hideCursor();
			}
			break;

		case KeyEvent::KEY_SPACE:
			if ( mState != STATE_GAME )
				startGame();
			break;

		case KeyEvent::KEY_F12:
			timeline().apply( &mCountdown, 0, 1.f ).finishFn( [ & ]() { mState = STATE_STATISTICS; } );
			break;

		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

void HeartRateApp::mouseDown( MouseEvent event )
{
}

void HeartRateApp::mouseDrag( MouseEvent event )
{
}

void HeartRateApp::resize()
{
}

CINDER_APP_BASIC( HeartRateApp, RendererGl( RendererGl::AA_NONE ) )


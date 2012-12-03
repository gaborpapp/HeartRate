#include "cinder/gl/gl.h"

#include "cinder/CinderMath.h"
#include "cinder/Easing.h"
#include "cinder/ImageIo.h"
#include "cinder/ObjLoader.h"

#include "HeartShape.h"

using namespace ci;
using namespace std;

HeartShape::HeartShape() :
	mColorInactive( ColorA::hexA( 0xff862c2c ) ),
	//mColorActive( ColorA::hexA( 0xffab0706 ) )
	mColorActive( ColorA::white() ),
	mDisplaceScale( 15.f ),
	mTextureEnabled( true ),
	mFboSize( Vec2i( 64, 64 ) )
{
}

void HeartShape::setup()
{
	ObjLoader loader = ObjLoader( app::loadAsset( "heart.obj" ) );
	loader.load( &mTriMesh );
	//mTriMesh.recalculateNormals();

	try
	{
		mShader = gl::GlslProg( app::loadAsset( "HeartVert.glsl" ),
								app::loadAsset( "HeartFrag.glsl" ) );
		mDisplacementShader = gl::GlslProg( app::loadAsset( "DisplaceVert.glsl" ),
											app::loadAsset( "DisplaceFrag.glsl" ) );
	}
	catch ( const gl::GlslProgCompileExc &exc )
	{
		app::console() << exc.what() << endl;
	}

	mMaterial = gl::Material( Color::gray( .0 ), Color::gray( .5 ), Color::white(), 50.f );
	mTexture = loadImage( app::loadAsset( "test.png" ) );

	gl::Fbo::Format format;
	format.setColorInternalFormat( GL_RGB32F_ARB );
	format.enableDepthBuffer( false );
	mFbo = gl::Fbo( mFboSize.x, mFboSize.y, format );
}

void HeartShape::update()
{
	gl::SaveFramebufferBinding bindingSaver;
	mFbo.bindFramebuffer();
	gl::setViewport( mFbo.getBounds() );
	gl::setMatricesWindow( mFbo.getSize(), false );
	mDisplacementShader.bind();
	mDisplacementShader.uniform( "theta", static_cast< float >( app::getElapsedSeconds() ) );
	gl::drawSolidRect( mFbo.getBounds() );
	mDisplacementShader.unbind();
}

void HeartShape::draw()
{
	gl::enableDepthRead();
	gl::enableDepthWrite();
	mTexture.bind();
	mFbo.getTexture().bind( 1 );
	mShader.bind();
	mShader.uniform( "displacement", 1 );
	mShader.uniform( "scale", mDisplaceScale );

	mShader.uniform( "tex", 0 );
	mShader.uniform( "textureEnabled", mTextureEnabled );
	mMaterial.apply();
	gl::draw( mTriMesh );
	mShader.unbind();
	mTexture.unbind();
	mFbo.getTexture().unbind( 1 );
}


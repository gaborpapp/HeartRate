#include "cinder/gl/gl.h"

#include "cinder/CinderMath.h"
#include "cinder/Easing.h"
#include "cinder/ImageIo.h"
#include "cinder/ObjLoader.h"

#include "HeartShape.h"

using namespace ci;
using namespace std;

HeartShape::HeartShape() :
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
		mDispMapShader = gl::GlslProg( app::loadAsset( "DisplaceVert.glsl" ),
									   app::loadAsset( "DisplaceFrag.glsl" ) );
		mNormalMapShader = gl::GlslProg( app::loadAsset( "NormalVert.glsl" ),
										 app::loadAsset( "NormalFrag.glsl" ) );
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
	mDispMapFbo = gl::Fbo( mFboSize.x, mFboSize.y, format );
	mNormalMapFbo = gl::Fbo( mFboSize.x, mFboSize.y, format );

	mParams = params::PInterfaceGl( "Heart", Vec2i( 300, 120 ), Vec2i( 326, 16 ) );
	mParams.addPersistentSizeAndPosition();

	//mParams.addPersistentParam( "Active color", &mColorActive, ColorA::hexA( 0xffab0706 ) );
	mParams.addPersistentParam( "Active color", &mColorActive, ColorA::white() );
	mParams.addPersistentParam( "Inactive color", &mColorInactive, ColorA::hexA( 0xff862c2c ) );
	mParams.addPersistentParam( "Displace scale", &mDisplaceScale, 15.f, "min=0 max=50" );
	mParams.addPersistentParam( "Normal amlitude", &mNormalAmplitude, 10.f, "min=1 max=50" );
	mParams.addPersistentParam( "Texture enable", &mTextureEnabled, true );
}

void HeartShape::update()
{
	mDispMapFbo.bindFramebuffer();
	gl::setViewport( mDispMapFbo.getBounds() );
	gl::setMatricesWindow( mDispMapFbo.getSize(), false );
	mDispMapShader.bind();
	mDispMapShader.uniform( "theta", static_cast< float >( app::getElapsedSeconds() ) );
	gl::drawSolidRect( mDispMapFbo.getBounds() );
	mDispMapShader.unbind();
	mDispMapFbo.unbindFramebuffer();

	mNormalMapFbo.bindFramebuffer();
	mNormalMapShader.bind();
	mNormalMapShader.uniform( "texture", 0 );
	mNormalMapShader.uniform( "amplitude", mNormalAmplitude );
	mDispMapFbo.getTexture().bind();
	gl::drawSolidRect( mNormalMapFbo.getBounds() );
	mDispMapFbo.getTexture().unbind();
	mNormalMapShader.unbind();
	mNormalMapFbo.unbindFramebuffer();
}

void HeartShape::draw()
{
	gl::enableDepthRead();
	gl::enableDepthWrite();
	mTexture.bind();
	mDispMapFbo.getTexture().bind( 1 );
	mShader.bind();
	mShader.uniform( "displacement", 1 );
	mShader.uniform( "scale", mDisplaceScale );

	mShader.uniform( "tex", 0 );
	mShader.uniform( "textureEnabled", mTextureEnabled );
	mMaterial.setDiffuse( mColorActive );
	mMaterial.apply();
	gl::draw( mTriMesh );
	mShader.unbind();
	mTexture.unbind();
	mDispMapFbo.getTexture().unbind( 1 );

	/*
	const std::vector< Vec3f > &v = mTriMesh.getVertices();
	const std::vector< Vec3f > &n = mTriMesh.getNormals();
	for ( size_t i = 0; i < n.size(); i++ )
	{
		gl::drawVector( v[ i ], v[ i ] + 10 * n[ i ] );
	}
	*/
}


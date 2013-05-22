#include "cinder/gl/gl.h"

#include "cinder/CinderMath.h"
#include "cinder/Easing.h"
#include "cinder/ImageIo.h"
#include "cinder/ObjLoader.h"

#include "HeartShape.h"

using namespace ci;
using namespace std;

HeartShape::HeartShape() :
	mFboSize( Vec2i( 64, 64 ) )
{
}

void HeartShape::setup()
{
	ObjLoader loader = ObjLoader( app::loadAsset( "model/heart.obj" ) );
	loader.load( &mTriMesh );
	//mTriMesh.recalculateNormals();

	try
	{
		mShader = gl::GlslProg( app::loadAsset( "shaders/HeartVert.glsl" ),
								app::loadAsset( "shaders/HeartFrag.glsl" ) );
		mDispMapShader = gl::GlslProg( app::loadAsset( "shaders/DisplaceVert.glsl" ),
									   app::loadAsset( "shaders/DisplaceFrag.glsl" ) );
		mNormalMapShader = gl::GlslProg( app::loadAsset( "shaders/NormalVert.glsl" ),
										 app::loadAsset( "shaders/NormalFrag.glsl" ) );
	}
	catch ( const gl::GlslProgCompileExc &exc )
	{
		app::console() << exc.what() << endl;
	}

	mMaterial = gl::Material( Color::gray( .0 ), Color::gray( .5 ), Color::white(), 50.f );
	mTexture = loadImage( app::loadAsset( "model/heart.png" ) );

	gl::Fbo::Format format;
	format.setColorInternalFormat( GL_RGB32F_ARB );
	format.enableDepthBuffer( false );
	mDispMapFbo = gl::Fbo( mFboSize.x, mFboSize.y, format );
	mNormalMapFbo = gl::Fbo( mFboSize.x, mFboSize.y, format );

	mParams = mndl::params::PInterfaceGl( "Heart", Vec2i( 300, 120 ), Vec2i( 326, 16 ) );
	mParams.addPersistentSizeAndPosition();

	mParams.addPersistentParam( "Active color", &mColorActive, ColorA::hexA( 0xffab0706 ) );
	//mParams.addPersistentParam( "Active color", &mColorActive, ColorA::white() );
	mParams.addPersistentParam( "Inactive color", &mColorInactive, ColorA::hexA( 0xff862c2c ) );
	mParams.addPersistentParam( "Displace scale", &mDisplaceScale, 15.f, "min=0 max=50 step=.1" );
	//mParams.addPersistentParam( "Normal amlitude", &mNormalAmplitude, 10.f, "min=1 max=50" );
	mParams.addPersistentParam( "3d enable", &m3dEnabled, true );
	//mParams.addPersistentParam( "Flat shading enable", &mFlatShadingEnabled, false );
	mParams.addPersistentParam( "Texture enable", &mTextureEnabled, false );
	mParams.addPersistentParam( "Heart enable", &mHeartEnabled, true );
	mParams.addPersistentParam( "Normals enable", &mNormalsEnabled, false );
	mParams.addSeparator();
	mDisharmony = 0.f;
	mParams.addParam( "Disharmony", &mDisharmony, "min=0 max=1 step=.05" );

	calculateTangents();
	setupVbo();
}

void HeartShape::calculateTangents()
{
	// calculate tangents
	const std::vector< Vec3f > &v = mTriMesh.getVertices();
	const std::vector< uint32_t > &ind = mTriMesh.getIndices();
	const std::vector< Vec2f > &texCoords = mTriMesh.getTexCoords();
	mTangents.resize( v.size() );
	for ( size_t i = 0; i < v.size(); i++ )
	{
		size_t i0 = ind[ i * 3 ];
		size_t i1 = ind[ i * 3 + 1 ];
		size_t i2 = ind[ i * 3 + 2 ];

		Vec3f p0 = v[ i0 ];
		Vec3f p1 = v[ i1 ];
		Vec3f p2 = v[ i2 ];

		Vec2f uv0 = texCoords[ i0 ];
		Vec2f uv1 = texCoords[ i1 ];
		Vec2f uv2 = texCoords[ i2 ];

		Vec3f v0 = p1 - p0;
		Vec3f v1 = p2 - p0;
		Vec2f st0 = uv1 - uv0;
		Vec2f st1 = uv2 - uv0;

		float coef = 1.f / ( st0.x * st1.y - st1.x * st0.y );
		Vec3f tangent( coef * ( v0.x * st1.y - st0.y * v1.x ),
					   coef * ( v0.y * st1.y - st0.y * v1.y ),
					   coef * ( v0.z * st1.y - st0.y * v1.z ) );

		tangent.normalize();
		mTangents[ i0 ] += tangent;
		mTangents[ i1 ] += tangent;
		mTangents[ i2 ] += tangent;
	}

	for ( std::vector< Vec3f >::iterator it = mTangents.begin(); it != mTangents.end(); ++it )
	{
		it->normalize();
	}
}

void HeartShape::setupVbo()
{
	gl::VboMesh::Layout layout;

	layout.setStaticPositions();
	layout.setStaticIndices();
	layout.setStaticNormals();
	layout.setStaticTexCoords2d();
	layout.mCustomStatic.push_back( std::make_pair( gl::VboMesh::Layout::CUSTOM_ATTR_FLOAT3, 0 ) ); // tangents

	size_t numVertices = mTriMesh.getNumVertices();

	mVboMesh = gl::VboMesh( numVertices, mTriMesh.getNumIndices(), layout, GL_TRIANGLES );

	mVboMesh.bufferIndices( mTriMesh.getIndices() );
	mVboMesh.bufferPositions( mTriMesh.getVertices() );
	mVboMesh.bufferNormals( mTriMesh.getNormals() );
	mVboMesh.bufferTexCoords2d( 0, mTriMesh.getTexCoords() );
	mVboMesh.unbindBuffers();

	mVboMesh.getStaticVbo().bind();
	size_t offset = sizeof( GLfloat ) * ( 3 + 3 + 2 ) * numVertices;
	mVboMesh.getStaticVbo().bufferSubData( offset,
			numVertices * sizeof( float ) * 3,
			&mTangents[ 0 ] );
	mVboMesh.getStaticVbo().unbind();

	mShader.bind();
	GLint locTangent = mShader.getAttribLocation( "tangent" );
	mVboMesh.setCustomStaticLocation( 0, locTangent );
	mShader.unbind();
}

void HeartShape::setAmplitudes( float amp0, float amp1 )
{
	mAmplitude0 = amp0;
	mAmplitude1 = amp1;
}

void HeartShape::update()
{
	mDispMapFbo.bindFramebuffer();
	gl::setViewport( mDispMapFbo.getBounds() );
	gl::setMatricesWindow( mDispMapFbo.getSize(), false );
	mDispMapShader.bind();
	mDispMapShader.uniform( "time", float( app::getElapsedSeconds() ) ); // FIXME: needed for distortion only
	mDispMapShader.uniform( "amplitude", 1.0f );
	mDispMapShader.uniform( "amp0", mAmplitude0 );
	mDispMapShader.uniform( "amp1", mAmplitude1 );
	mDispMapShader.uniform( "disharmony", mDisharmony ); // FIXME: needed for distortion only

	gl::drawSolidRect( mDispMapFbo.getBounds() );
	mDispMapShader.unbind();
	mDispMapFbo.unbindFramebuffer();

	mNormalMapFbo.bindFramebuffer();
	mNormalMapShader.bind();
	mNormalMapShader.uniform( "texture", 0 );
	//mNormalMapShader.uniform( "amplitude", mNormalAmplitude );
	// NOTE: hack to scale the normals, but gives ok results
	mNormalMapShader.uniform( "amplitude", mDisplaceScale );
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
	if ( mHeartEnabled )
	{
		mTexture.bind();
		mDispMapFbo.getTexture().bind( 1 );
		mNormalMapFbo.getTexture().bind( 2 );
		mShader.bind();
		mShader.uniform( "displacement", 1 );
		mShader.uniform( "normalMap", 2 );
		mShader.uniform( "scale", mDisplaceScale );

		mShader.uniform( "tex", 0 );
		mShader.uniform( "textureEnabled", mTextureEnabled );
		mShader.uniform( "no3dEnabled", !m3dEnabled );
		//mShader.uniform( "flat", mFlatShadingEnabled );
		mMaterial.setDiffuse( mColorActive );
		mMaterial.apply();
		gl::color( Color::white() );
		gl::draw( mVboMesh );
		mShader.unbind();
		mTexture.unbind();
		mDispMapFbo.getTexture().unbind( 1 );
		mNormalMapFbo.getTexture().unbind( 2 );
	}

	if ( mNormalsEnabled )
	{
		const std::vector< Vec3f > &v = mTriMesh.getVertices();
		const std::vector< Vec3f > &n = mTriMesh.getNormals();
		for ( size_t i = 0; i < n.size(); i++ )
		{
			gl::color( Color( 1, 0, 0 ) );
			gl::drawVector( v[ i ], v[ i ] + 10 * n[ i ] );
			gl::color( Color( 1, 0, 1 ) );
			gl::drawVector( v[ i ] + n[ i ], v[ i ] + n[ i ] + 10 * mTangents[ i ] );
		}
	}

	// FIXME: why is this not required? params are drawn irrespectively of
	// calling their draw() explicitly.
	//mParams.draw();
}


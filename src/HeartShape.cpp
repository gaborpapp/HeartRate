#include "cinder/gl/gl.h"

#include "cinder/CinderMath.h"
#include "cinder/Easing.h"
#include "cinder/ImageIo.h"
#include "cinder/ObjLoader.h"

#include "HeartShape.h"

using namespace ci;
using namespace std;

void HeartShape::setup()
{
	ObjLoader loader = ObjLoader( app::loadAsset( "model/heart6.obj" ) );
	loader.load( &mModelHeart );

	try
	{
		mShader = gl::GlslProg( app::loadAsset( "shaders/PhongDirectional.vert" ),
								app::loadAsset( "shaders/PhongDirectional.frag" ) );
	}
	catch ( const gl::GlslProgCompileExc &exc )
	{
		app::console() << exc.what() << endl;
	}
	catch ( const app::AssetLoadExc &exc )
	{
		app::console() << "Could not find asset: " << exc.what() << endl;
	}

	mMaterial = gl::Material( Color::gray( .0 ), Color::gray( .5 ), Color::white(), 50.f );
	//mTexture = loadImage( app::loadAsset( "model/heart.png" ) );

	mParams = mndl::params::PInterfaceGl( "Heart", Vec2i( 300, 120 ), Vec2i( 326, 16 ) );
	mParams.addPersistentSizeAndPosition();

	mParams.addPersistentParam( "Displace scale", &mDisplaceScale, 15.f, "min=0 max=50 step=.1" );
	mParams.addSeparator();

	mParams.addPersistentParam( "Light direction", &mLightDirection,
			Vec3f( -0.8, -1.07, -1.1 ).normalized() );
	mParams.addPersistentParam( "Light ambient", &mLightAmbient, ColorA::gray( .5f ) );
	mParams.addPersistentParam( "Light diffuse", &mLightDiffuse, ColorA::gray( .7f ) );
	mParams.addPersistentParam( "Light specular", &mLightSpecular, ColorA::white() );
	mParams.addSeparator();
	mParams.addPersistentParam( "Material ambient", &mMaterialAmbient, ColorA( Color8u( 153, 76, 78 ) ) );
	mParams.addPersistentParam( "Material diffuse", &mMaterialDiffuse, ColorA( ColorA8u( 254, 0, 7 ) ) );
	mParams.addPersistentParam( "Material specular", &mMaterialSpecular, ColorA::white() );
	mParams.addPersistentParam( "Material shininess", &mMaterialShininess, 20.f, "min=0.1 max=100 step=.1" );

	// light
    mLight = shared_ptr< gl::Light >( new gl::Light( gl::Light::DIRECTIONAL, 0 ) );
}

void HeartShape::setAmplitudes( float amp0, float amp1 )
{
	mAmplitude0 = amp0;
	mAmplitude1 = amp1;
}

void HeartShape::update( const Camera &camera )
{
    mLight->setAmbient( mLightAmbient );
    mLight->setDiffuse( mLightDiffuse );
    mLight->setSpecular( mLightSpecular );
    mLight->setDirection( -mLightDirection );
	mLight->update( camera );

	const size_t DISP_SIZE = 256;
	float xDisp[ DISP_SIZE ];
	float yDisp[ DISP_SIZE ];
	auto gauss = []( float sig, float mu, float x )
	{
		    return 1.f / ( sig * math< float >::sqrt( 2.f * M_PI ) ) *
				math< float >::exp( - 3.f * ( x - mu ) * ( x - mu ) / ( 2.f * sig * sig ) );
	};
	auto smoothstep = []( float edge0, float edge1, float x ) -> float
	{
		float t = math< float >::clamp( ( x - edge0 ) / ( edge1 - edge0 ) );
		return t * t * ( 3.f - 2.f * t );
	};

	float xAdd = 1.f / DISP_SIZE;
	float x = 0.f;
	for ( size_t t = 0; t < DISP_SIZE; t++, x += xAdd )
	{
		float b0 = mAmplitude0 * gauss( .7f, .25f, x );
		float b1 = mAmplitude1 * gauss( .7f, .75f, x );
		xDisp[ t ] = mDisplaceScale * lerp( b0, b1, smoothstep( .2f, .8f, x ) );
		yDisp[ t ] = math< float >::sin( M_PI * x );
	}
	Channel32f xDispChn = Channel32f( DISP_SIZE, 1, DISP_SIZE, 1, xDisp );
	if ( mDisplaceTexture )
		mDisplaceTexture.update( xDispChn );
	else
		mDisplaceTexture = gl::Texture( xDispChn );

	mModelHeartBeating.clear();
	const vector< Vec3f > & origVertices = mModelHeart.getVertices();
	const vector< Vec3f > & origNormals = mModelHeart.getNormals();
	const vector< Vec2f > & origTexCoords = mModelHeart.getTexCoords();

	auto vIt = origVertices.cbegin();
	auto nIt = origNormals.cbegin();
	auto tIt = origTexCoords.cbegin();
	for ( ; vIt != origVertices.cend(); ++vIt, ++nIt, ++tIt )
	{
		int s = int( 255.f * tIt->x );
		int t = int( 255.f * tIt->y );
		mModelHeartBeating.appendVertex( *vIt + yDisp[ t ] * xDisp[ s ] * 10 * ( *nIt ) );
	}

	mModelHeartBeating.appendNormals( &origNormals[ 0 ], origNormals.size() );
	mModelHeartBeating.appendTexCoords( &origTexCoords[ 0 ], origTexCoords.size() );
	mModelHeartBeating.appendIndices( &mModelHeart.getIndices()[ 0 ], mModelHeart.getNumIndices() );
	mModelHeartBeating.recalculateNormals();
}

void HeartShape::draw()
{
	gl::enableDepthRead();
	gl::enableDepthWrite();

	gl::enable( GL_LIGHTING );

	gl::Material material( mMaterialAmbient, mMaterialDiffuse, mMaterialSpecular,
			mMaterialShininess );
	material.apply();
    mLight->enable();

	gl::color( Color::white() );
	if ( mShader )
		mShader.bind();
	gl::draw( mModelHeartBeating );
	if ( mShader )
		mShader.unbind();
    mLight->disable();

	gl::disable( GL_LIGHTING );
	gl::disableDepthRead();
	gl::disableDepthWrite();
}


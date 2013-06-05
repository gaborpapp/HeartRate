#pragma once

#include <vector>

#include "cinder/app/App.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/Material.h"
#include "cinder/gl/Texture.h"
#include "cinder/Area.h"
#include "cinder/Camera.h"
#include "cinder/Color.h"
#include "cinder/TriMesh.h"
#include "cinder/Vector.h"

#include "mndlkit/params/PParams.h"

class HeartShape
{
	public:
		void setup();
		void update( const ci::Camera &camera );
		void draw();

		void setAmplitudes( float amp0, float amp1 );

		const ci::gl::Texture &getDisplacementTexture() { return mDisplaceTexture; }

	protected:
		float mDisplaceScale;
		//bool mTextureEnabled;

		float mAmplitude0;
		float mAmplitude1;

		ci::TriMesh mModelHeart;
		ci::TriMesh mModelHeartBeating;

		ci::gl::GlslProg mShader;
		ci::gl::Material mMaterial;

		ci::gl::Texture mDisplaceTexture;

		ci::ColorA mLightAmbient, mLightDiffuse, mLightSpecular;
		ci::Vec3f mLightDirection;
		bool mRecalcNormals;

		ci::ColorA mMaterialAmbient, mMaterialDiffuse, mMaterialSpecular;
		float mMaterialShininess;

		std::shared_ptr< ci::gl::Light > mLight;

		mndl::params::PInterfaceGl mParams;
};


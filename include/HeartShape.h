#pragma once

#include <vector>

#include "cinder/app/App.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Material.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"
#include "cinder/Area.h"
#include "cinder/Color.h"
#include "cinder/TriMesh.h"
#include "cinder/Vector.h"

#include "mndlkit/params/PParams.h"

class HeartShape
{
	public:
		HeartShape();

		void setup();
		void update();
		void draw();

		void setAmplitudes( float amp0, float amp1 );

		const ci::gl::Texture &getDisplacementTexture() { return mDispMapFbo.getTexture(); }
		const ci::gl::Texture &getNormalTexture() { return mNormalMapFbo.getTexture(); }

	protected:
		ci::ColorA mColorInactive;
		ci::ColorA mColorActive;
		float mDisplaceScale;
		//float mNormalAmplitude;
		bool mTextureEnabled;
		bool mHeartEnabled;
		bool mNormalsEnabled;
		ci::Vec2i mFboSize;

		float mAmplitude0;
		float mAmplitude1;

		float mDisharmony;

		ci::TriMesh mTriMesh;

		void calculateTangents();
		std::vector< ci::Vec3f > mTangents;

		void setupVbo();
		ci::gl::VboMesh mVboMesh;

		ci::gl::GlslProg mShader;
		ci::gl::Material mMaterial;
		ci::gl::Texture mTexture;

		ci::gl::Fbo mDispMapFbo;
		ci::gl::GlslProg mDispMapShader;

		ci::gl::Fbo mNormalMapFbo;
		ci::gl::GlslProg mNormalMapShader;

		mndl::kit::params::PInterfaceGl mParams;
};


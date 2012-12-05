#pragma once

#include "cinder/app/App.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Material.h"
#include "cinder/gl/Texture.h"
#include "cinder/Area.h"
#include "cinder/Color.h"
#include "cinder/TriMesh.h"
#include "cinder/Vector.h"

#include "PParams.h"

class HeartShape
{
	public:
		HeartShape();

		void setup();
		void update();
		void draw();

		const ci::gl::Texture &getDisplacementTexture() { return mDispMapFbo.getTexture(); }
		const ci::gl::Texture &getNormalTexture() { return mNormalMapFbo.getTexture(); }

	protected:
		ci::ColorA mColorInactive;
		ci::ColorA mColorActive;
		float mDisplaceScale;
		float mNormalAmlitude;
		bool mTextureEnabled;
		ci::Vec2i mFboSize;

		ci::TriMesh mTriMesh;

		ci::gl::GlslProg mShader;
		ci::gl::Material mMaterial;
		ci::gl::Texture mTexture;

		ci::gl::Fbo mDispMapFbo;
		ci::gl::GlslProg mDispMapShader;

		ci::gl::Fbo mNormalMapFbo;
		ci::gl::GlslProg mNormalMapShader;

		ci::params::PInterfaceGl mParams;
};


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

class HeartShape
{
	public:
		HeartShape();

		void setup();
		void update();
		void draw();

		void setColorInactive( const ci::ColorA &color ) { mColorInactive = color; }
		void setColorActive( const ci::ColorA &color ) { mMaterial.setDiffuse( color ); }
		void setDisplacementScale( float scale ) { mDisplaceScale = scale; }

		void enableTexture( bool enable = true ) { mTextureEnabled = enable; }

		const ci::gl::Texture &getDisplacementTexture() { return mFbo.getTexture(); }

	protected:
		ci::ColorA mColorInactive;
		ci::ColorA mColorActive;
		float mDisplaceScale;
		bool mTextureEnabled;
		ci::Vec2i mFboSize;

		ci::TriMesh mTriMesh;

		ci::gl::GlslProg mShader;
		ci::gl::Material mMaterial;
		ci::gl::Texture mTexture;

		ci::gl::Fbo mFbo;
		ci::gl::GlslProg mDisplacementShader;
};


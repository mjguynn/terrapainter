#pragma once
#include "terrapainter/math.h"
#include "../shadermgr.h"

class ScreenspaceCircleShader {
public:
	ScreenspaceCircleShader(int viewWidth, int viewHeight);
	void use(vec2 center, float radius, float thickness);
private:
	GLuint mProgram;
	GLuint mCenterLocation;
	GLuint mScreenSizeLocation;
	GLuint mRadiusLocation;
	GLuint mThicknessLocation;

	int mViewWidth;
	int mViewHeight;
};
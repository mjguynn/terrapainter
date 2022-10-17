#include "screenspace_circle.h"
#include "SDL.h"
#include "SDL_opengl.h"

ScreenspaceCircleShader::ScreenspaceCircleShader(int viewWidth, int viewHeight)
	: mProgram(g_shaderMgr.graphics("screenspace_circle")), 
	mCenterLocation(), // Temp value 
	mScreenSizeLocation(), // Temp value
	mRadiusLocation(), // Temp value
	mThicknessLocation(), // Temp value
	mViewWidth(viewWidth), // Temp value (on_size_changed)
	mViewHeight(viewHeight) // Temp value (on_size_changed)
{
	glUseProgram(mProgram);
	mCenterLocation = glGetUniformLocation(mProgram, "u_center");
	mScreenSizeLocation = glGetUniformLocation(mProgram, "u_screenSize");
	mRadiusLocation = glGetUniformLocation(mProgram, "u_radius");
	mThicknessLocation = glGetUniformLocation(mProgram, "u_thickness");
}
void ScreenspaceCircleShader::use(vec2 center, float radius, float thickness) {
	
	glUseProgram(mProgram);
	glUniform2f(mCenterLocation, center.x, center.y);
	glUniform2i(mScreenSizeLocation, mViewWidth, mViewHeight);
	glUniform1f(mRadiusLocation, radius);
	glUniform1f(mThicknessLocation, thickness);
}
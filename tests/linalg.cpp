#include <catch2/catch_test_macros.hpp>
#include "terrapainter/linalg.h"
TEST_CASE("Vec4 constructors/splats", "[linalg]") {
	vec4 splat = vec4::splat(7.0);
	REQUIRE( (splat.f.x == 7.0 && splat[0] == 7.0) );
	REQUIRE( (splat.f.y == 7.0 && splat[1] == 7.0) );
	REQUIRE( (splat.f.z == 7.0 && splat[2] == 7.0) );
	REQUIRE( (splat.f.w == 7.0 && splat[3] == 7.0) );

	vec4 custom{ 1.0, 5.0, 0.5, 25.0 };
	REQUIRE( (custom.f.x == 1.0 && custom[0] == 1.0) );
	REQUIRE( (custom.f.y == 5.0 && custom[1] == 5.0) );
	REQUIRE( (custom.f.z == 0.5 && custom[2] == 0.5) );
	REQUIRE( (custom.f.w == 25.0 && custom[3] == 25.0) );
}
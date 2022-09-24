#include <catch2/catch_test_macros.hpp>
#include "terrapainter/math.h"
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

	vec4 custom2 = { 1, 4, 3, 2 };
	REQUIRE( (custom2.f.x == 1.0 && custom2[0] == 1.0) );
	REQUIRE( (custom2.f.y == 4.0 && custom2[1] == 4.0) );
	REQUIRE( (custom2.f.z == 3.0 && custom2[2] == 3.0) );
	REQUIRE( (custom2.f.w == 2.0 && custom2[3] == 2.0) );
}
TEST_CASE("Vec4 comparisons", "[linalg]") {
	SECTION("Zero Vector") {
		REQUIRE(vec4::splat(0.0) == vec4{ 0, 0, 0, 0 });
	}
	SECTION("Random Vectors") {
		REQUIRE(vec4{ 5.0, 0.0, 0.0, 0.0 } != vec4{ 5.0, 0.0, 0.0, 7.0 });
		REQUIRE(vec4{ 0.0, 0.0, 0.0, -7.0 } != vec4{ 0.0, 0.0, 0.0, 7.0 });
		REQUIRE(vec4{ 123.5, 86.2, 97.5, 12.0 } != vec4{ 36.4, 872.0, 23.0, 47.0 });
		REQUIRE(vec4{ 143.6, -127.0, 23.0, -54.0 } == vec4{ 143.6, -127.0, 23.0, -54.0 });
	}
	SECTION("NaN") {
		REQUIRE(vec4{ 7.f, 7.f, 7.f, 12.f } != vec4{ 7.f, 7.f, 7.f, NAN });
		REQUIRE(vec4{ 9.f, 9.f, 9.f, 9.f } != vec4{ NAN, NAN, NAN, NAN });
		// yes -- NaN compares false with EVERY float, including itself!
		REQUIRE(vec4{ NAN, NAN, NAN, NAN } != vec4{ NAN, NAN, NAN, NAN });
	}
	SECTION("Infinity") {
		REQUIRE(vec4{ 7.f, 7.f, 7.f, 12.f } != vec4{ 7.f, 7.f, 7.f, INFINITY });
		REQUIRE(vec4{ 7.f, 7.f, 7.f, INFINITY } != vec4{ 7.f, 7.f, 7.f, -INFINITY });
		REQUIRE(vec4{ 7.f, 7.f, 7.f, INFINITY } == vec4{ 7.f, 7.f, 7.f, INFINITY });
	}
	SECTION("Precision") {
		vec4 target = { 27, 30, 92, -83 };
		vec4 offset = { 27 + FLT_EPSILON, 30 - FLT_EPSILON, 92 - FLT_EPSILON, -83 + 2 * FLT_EPSILON };
		REQUIRE(target == offset);
		vec4 too_much_offset = { 27 + FLT_EPSILON, 29.99f, 92.0f, -83.0f };
		REQUIRE(target != too_much_offset);
	}
}
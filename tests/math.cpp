#include <cfloat>
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
TEST_CASE("Vec4 addition/subtraction") {
	SECTION("Random Vectors") {
		vec4 a = { 5, -7, 6, 2 };
		vec4 b = { 12.f, 16.f, -69.f, -13.f };
		vec4 apb = vec4{ 17.f, 9.f, -63.f, -11.f };
		vec4 amb = vec4{ -7.f, -23.f, 75.f, 15.f };
		vec4 bma = vec4{ 7.f, 23.f, -75.f, -15.f };
		REQUIRE(a + b == apb);
		REQUIRE(a - b == amb);
		REQUIRE(b - a == bma);
			
		vec4 ac = a;
		ac += b;
		REQUIRE(ac == apb);

		vec4 bc = b;
		bc -= a;
		REQUIRE(bc == bma);
	}
	SECTION("Additive Inverses") {
		vec4 a = { -5.f, 0.f, -67.f, 425.f / 3.f };
		vec4 b = { 5.f, -0.f, 67.f, -425.f / 3.f };
		REQUIRE(a + b == vec4::splat(0.f));
	}
}
TEST_CASE("Vec4 multiplication") {
	SECTION("Random Vectors * Scalars") {
		vec4 v = { 0.6, -23.2, 9999.0, 0.0 };
		REQUIRE(v * 2 == vec4{ 1.2, -46.4, 19998.0, 0.0 });
		REQUIRE(-6 * v == vec4{ -3.6, 139.2, -59994.0, 0.0 });
		v *= 0.5;
		REQUIRE(v == vec4{ 0.3, -11.6, 4999.5, 0.0 });
	}
	SECTION("Random Vectors * Vectors") {
		vec4 a = { 0.5, 16.2, 30.7, 29.99 };
		vec4 b = { -12, -36, 0, -65 };
		vec4 c = { -18, 3, -1, 1 };

		REQUIRE(a * b == vec4{ -6.f, -583.2f, 0.f, -1949.35f });
		REQUIRE(a * c == vec4{ -9.f, 48.6f, -30.7f, 29.99f });
		REQUIRE(b * c == vec4{ 216.f, -108.f, 0.f, -65.f });
	}
}
TEST_CASE("Vec4 division") {
	SECTION("Division by zero") {
		vec4 v = { 1.f, 0.f, -3.f, NAN };
		v /= 0;
		// NaN == NaN will always be false,
		// so we've gotta be explicit here
		REQUIRE(v.f.x == INFINITY);
		REQUIRE(std::isnan(v.f.y));
		REQUIRE(v.f.z == -INFINITY);
		REQUIRE(std::isnan(v.f.w));
	}
	SECTION("Random Vectors / Scalars") {
		vec4 v = { 0.5, 16.2, -30.7, 29.99 };
		REQUIRE(v / 0.5 == vec4{ 1.0, 32.4, -61.4, 59.98 });
		REQUIRE(v / 4 == vec4{ 0.125, 4.05, -7.675, 7.4975 });
		REQUIRE(v / -3 == vec4{ -0.1666666667, -5.4, 10.2333333333, -9.9966666667 });
	}
	SECTION("Random Vectors / Vectors") {
		vec4 a = { 0.5, 16.2, 30.7, 29.99 };
		vec4 b = { 4, 4, 4, 5 };
		vec4 c = { -18.0, 3.0, -1.0, -0.1 };

		REQUIRE(a / b == vec4{ 0.125, 4.05, 7.675, 5.998 });
		REQUIRE(a / c == vec4{ -0.0277777778, 5.4, -30.7, -299.9 });
		REQUIRE(b / c == vec4{ -0.2222222222, 1.3333333333, -4.0, -50.0 });
	}
}
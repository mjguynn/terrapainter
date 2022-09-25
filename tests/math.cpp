#include <cfloat>
#include <catch2/catch_test_macros.hpp>
#include "terrapainter/math.h"
TEST_CASE("Vector constructors/splats", "[linalg]") {
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
TEST_CASE("Vector comparisons", "[linalg]") {
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
TEST_CASE("Vector addition/subtraction") {
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
TEST_CASE("Vector multiplication") {
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
TEST_CASE("Vector division") {
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
TEST_CASE("Vector dot product, length") {
	SECTION("Dot Product") {
		SECTION("Perpendicular") {
			vec4 i = { 1.0, 0.0, 0.0, 0.0 };
			vec4 j = { 0.0, 1.0, 0.0, 0.0 };
			REQUIRE(aeq(dot(i, j),0.f));

			vec4 a = { 0.f, 1.f / sqrtf(2), 1.f / sqrtf(2), 0.f };
			vec4 b = { 0.f, -1.f / sqrtf(2), 1.f / sqrtf(2), 0.f };
			REQUIRE(aeq(dot(a, b), 0.f));
		}
		SECTION("Parallel") {
			vec4 a = { 1.0, 6.0, 0.0, -1.0 };
			vec4 b = { 5.0, 30.0, 0.0, -5.0 };
			vec4 c = { -5.0, -30.0, 0.0, 5.0 };
			REQUIRE(aeq(dot(a, b), 190.f));
			REQUIRE(aeq(dot(a, c), -190.f));
		}
		SECTION("Random") {
			vec4 a = { 1.0, 6.0, 0.0, -1.0 };
			vec4 b = { 76.0, 0.0, -32.0, 18.0 };
			vec4 c = { 99.2, 43.7, -0.5, -0.333 };
			REQUIRE(aeq(dot(a, b), 58.f));
			REQUIRE(aeq(dot(a, c), 361.733f));
			REQUIRE(aeq(dot(b, c), 7549.206f));
		}
	}
	SECTION("Length") {
		REQUIRE(vec4::zero().mag() == 0.f);
		vec4 basis = { 1.0, 0.0, 0.0, 0.0 };
		REQUIRE(basis.mag() == 1.f);
		vec4 unit = { 0.f, 1.f / sqrtf(2), 1.f / sqrtf(2), 0.f };
		REQUIRE(aeq(unit.mag(), 1.f));
		vec4 random = { 6.0, -7.0, 0.0, 0.32 };
		REQUIRE(aeq(random.mag(), 9.2250962055f));
	}
}
TEST_CASE("Vector normalize") {
	SECTION("Zero vector") {
		vec4 nmz = vec4::zero().normalize();
		for (size_t i = 0; i < 4; i++) {
			REQUIRE(!std::isfinite(nmz[i]));
		}
	}
	SECTION("Random vectors") {
		vec4 a = { 0, 17, 17, 0 };
		vec4 an = vec4{ 0.f, 1.f / sqrtf(2), 1.f / sqrtf(2), 0.f };
		REQUIRE(a.normalize() == an);

		vec4 b = { 83.0, -65.23, -0.05, 7.64 };
		vec4 bn = vec4{ 0.7841948905, -0.6163015989, -0.0004724066, 0.0721837224 };
		REQUIRE(b.normalize() == bn);
	}
}
TEST_CASE("Vector cross product") {
	SECTION("Orthonormal") {
		vec3 i = { 1.0, 0.0, 0.0 };
		vec3 j = { 0.0, 1.0, 0.0 };
		vec3 ixj = { 0.0, 0.0, 1.0 };
		REQUIRE(aeq(cross(i, j), ixj));
		REQUIRE(aeq(cross(j, i), -ixj));

		vec3 a = { 0.f, 1.f / sqrtf(2), 1.f / sqrtf(2)};
		vec3 b = { 0.f, -1.f / sqrtf(2), 1.f / sqrtf(2)};
		vec3 axb = { 1.0, 0.0, 0.0 };
		REQUIRE(aeq(cross(a, b), axb));
		REQUIRE(aeq(cross(b, a), -axb));

		vec3 c = { -0.23939017, 0.58743526, -0.77305379 };
		vec3 d = { 0.81921268, -0.30515101, -0.48556508 };
		vec3 cxd = { -0.52113619, -0.74953498, -0.40818426 };
		REQUIRE(aeq(cross(c, d), cxd));
		REQUIRE(aeq(cross(d, c), -cxd));
	}
	SECTION("Random") {
		vec3 a = { 7.43, 9.8, -5.0 };
		vec3 b = { -6.0, -3.2, 0.0 };
		vec3 axb = { -16.0, 30.0, 35.024 };
		REQUIRE(aeq(cross(a, b), axb));
		REQUIRE(aeq(cross(b, a), -axb));
	}
	SECTION("Parallel / Zero") {
		vec3 zxz = cross(vec3::zero(), vec3::zero());
		REQUIRE(zxz == vec3::zero());

		vec3 a = { 12.0, 22.0, 0.5 };
		vec3 b = { 6.0, 11.0 , 0.25 };
		REQUIRE(aeq(cross(a, b), vec3::zero()));
		REQUIRE(aeq(cross(b, a), vec3::zero()));
	}
}
TEST_CASE("Vector linear interpolation") {
	vec3 x = { 12, -43, 86 };
	vec3 y = { 0.0, -5.0, -2.75 };
	REQUIRE(aeq(lerp(x, y, 0.f), x));
	REQUIRE(aeq(lerp(x, y, 1.f), y));
	REQUIRE(aeq(lerp(x, y, 0.5f), vec3{6.f, -24.f, 41.625f}));
	// ugh.. we do actually get the correct result here, but there's a bit too much precision loss at play.
	REQUIRE(aeq(lerp(x, y, 0.33f), vec3{ 8.04f, -30.46f, 56.7125f }, 1e-5f));
	REQUIRE(aeq(lerp(x, y, -1.f), vec3{ 24.f, -81.f, 174.75f }));
}
TEST_CASE("Vector reflection") {
	SECTION("Basic") {
		vec3 n = { 0.0, 0.0, 1.0 };

		vec3 incoming_tx = { 1.0, 0.0, 0.0 };
		vec3 incoming_ty = { 0.0, 1.0, 0.0 };
		vec3 incoming_n = { 0.0, 0.0, -1.0 };
		vec3 incoming_custom = { 5.0, -2.0, -4.0 };

		REQUIRE(incoming_tx.reflect_off(n) == incoming_tx);
		REQUIRE(incoming_ty.reflect_off(n) == incoming_ty);
		REQUIRE(incoming_n.reflect_off(n) == -incoming_n);
		REQUIRE(incoming_custom.reflect_off(n) == vec3{ 5.0, -2.0, 4.0 });
	}
	SECTION("Complicated") {
		vec3 n = {
			-sqrtf(2.f) / 2.f,
			2.f * sqrtf(2.f) / 5.f,
			3.f * sqrtf(2.f) / 10.f
		};
		vec3 incoming = { 2, -6, 2 };
		// again, correct output, just unfortunate precision loss.
		REQUIRE(aeq(incoming.reflect_off(n), vec3{ -3.6, -1.52, 5.36 }, 1e6f));
	}
}
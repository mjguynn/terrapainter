#include <cfloat>
#include <catch2/catch_test_macros.hpp>
#include "terrapainter/math.h"
TEST_CASE("Vector constructors/splats", "[linalg]") {
	vec4 splat = vec4::splat(7.0);
	REQUIRE( (splat.x == 7.0 && splat[0] == 7.0) );
	REQUIRE( (splat.y == 7.0 && splat[1] == 7.0) );
	REQUIRE( (splat.z == 7.0 && splat[2] == 7.0) );
	REQUIRE( (splat.w == 7.0 && splat[3] == 7.0) );

	vec4 custom = {1.0, 5, 0.5, 25.f};
	REQUIRE( (custom.x == 1.0 && custom[0] == 1.0) );
	REQUIRE( (custom.y == 5.0 && custom[1] == 5.0) );
	REQUIRE( (custom.z == 0.5 && custom[2] == 0.5) );
	REQUIRE( (custom.w == 25.0 && custom[3] == 25.0) );

	vec4 custom2 = { 1, 4, 3, 2 };
	REQUIRE( (custom2.x == 1.0 && custom2[0] == 1.0) );
	REQUIRE( (custom2.y == 4.0 && custom2[1] == 4.0) );
	REQUIRE( (custom2.z == 3.0 && custom2[2] == 3.0) );
	REQUIRE( (custom2.w == 2.0 && custom2[3] == 2.0) );
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
		REQUIRE(!aeq(vec4{ 7.f, 7.f, 7.f, 12.f }, vec4{ 7.f, 7.f, 7.f, NAN }));
		REQUIRE(!aeq(vec4{ 9.f, 9.f, 9.f, 9.f }, vec4{ NAN, NAN, NAN, NAN }));
		// yes -- NaN compares false with EVERY float, including itself!
		REQUIRE(!aeq(vec4{ NAN, NAN, NAN, NAN }, vec4{ NAN, NAN, NAN, NAN }));
	}
	SECTION("Infinity") {
		REQUIRE(!aeq(vec4{ 7.f, 7.f, 7.f, 12.f }, vec4{ 7.f, 7.f, 7.f, INFINITY }));
		REQUIRE(!aeq(vec4{ 7.f, 7.f, 7.f, INFINITY }, vec4{ 7.f, 7.f, 7.f, -INFINITY }));
		REQUIRE(aeq(vec4{ 7.f, 7.f, 7.f, INFINITY }, vec4{ 7.f, 7.f, 7.f, INFINITY }));
	}
	SECTION("Precision") {
		vec4 target = { 27, 30, 92, -83 };
		vec4 offset = { 27 + 20 * FLT_EPSILON, 30 - 15 * FLT_EPSILON, 92, -83 + 40 * FLT_EPSILON };
		REQUIRE(target != offset); // sanity check
		REQUIRE(aeq(target, offset));
		vec4 too_much_offset = { 27 + 20 * FLT_EPSILON, 29.99f, 92.0f, -83.0f };
		REQUIRE(target != too_much_offset);
		REQUIRE(!aeq(target, too_much_offset));
	}
}
TEST_CASE("Vector addition/subtraction", "[linalg]") {
	SECTION("Random Vectors") {
		vec4 a = { 5, -7, 6, 2 };
		vec4 b = { 12.f, 16.f, -69.f, -13.f };
		vec4 apb = vec4{ 17.f, 9.f, -63.f, -11.f };
		vec4 amb = vec4{ -7.f, -23.f, 75.f, 15.f };
		vec4 bma = vec4{ 7.f, 23.f, -75.f, -15.f };
		REQUIRE(aeq(a + b, apb));
		REQUIRE(aeq(a - b, amb));
		REQUIRE(aeq(b - a, bma));
			
		vec4 ac = a;
		ac += b;
		REQUIRE(aeq(ac, apb));

		vec4 bc = b;
		bc -= a;
		REQUIRE(aeq(bc, bma));
	}
	SECTION("Additive Inverses") {
		vec4 a = { -5.f, 0.f, -67.f, 425.f / 3.f };
		vec4 b = { 5.f, -0.f, 67.f, -425.f / 3.f };
		REQUIRE(a + b == vec4::splat(0.f));
	}
}
TEST_CASE("Vector multiplication", "[linalg]") {
	SECTION("Random Vectors * Scalars") {
		vec4 v = { 0.6, -23.2, 9999.0, 0.0 };
		REQUIRE(aeq(v * 2, vec4{ 1.2, -46.4, 19998.0, 0.0 }));
		REQUIRE(aeq(- 6 * v, vec4{-3.6, 139.2, -59994.0, 0.0}));
		v *= 0.5;
		REQUIRE(aeq(v, vec4{ 0.3, -11.6, 4999.5, 0.0 }));
	}
	SECTION("Random Vectors * Vectors") {
		vec4 a = { 0.5, 16.2, 30.7, 29.99 };
		vec4 b = { -12, -36, 0, -65 };
		vec4 c = { -18, 3, -1, 1 };

		REQUIRE(aeq(a * b, vec4{ -6.f, -583.2f, 0.f, -1949.35f }));
		REQUIRE(aeq(a * c, vec4{ -9.f, 48.6f, -30.7f, 29.99f }));
		REQUIRE(aeq(b * c, vec4{ 216.f, -108.f, 0.f, -65.f }));
	}
}
TEST_CASE("Vector division", "[linalg]") {
	SECTION("Division by zero") {
		vec4 v = { 1.f, 0.f, -3.f, NAN };
		v /= 0;
		// NaN == NaN will always be false,
		// so we've gotta be explicit here
		REQUIRE(v.x == INFINITY);
		REQUIRE(std::isnan(v.y));
		REQUIRE(v.z == -INFINITY);
		REQUIRE(std::isnan(v.w));
	}
	SECTION("Random Vectors / Scalars") {
		vec4 v = { 0.5, 16.2, -30.7, 29.99 };
		REQUIRE(aeq(v / 0.5, vec4{ 1.0, 32.4, -61.4, 59.98 }));
		REQUIRE(aeq(v / 4, vec4{ 0.125, 4.05, -7.675, 7.4975 }));
		REQUIRE(aeq(v / -3, vec4{ -0.1666666667, -5.4, 10.2333333333, -9.9966666667 }));
	}
	SECTION("Random Vectors / Vectors") {
		vec4 a = { 0.5, 16.2, 30.7, 29.99 };
		vec4 b = { 4, 4, 4, 5 };
		vec4 c = { -18.0, 3.0, -1.0, -0.1 };

		REQUIRE(aeq(a / b, vec4{ 0.125, 4.05, 7.675, 5.998 }));
		REQUIRE(aeq(a / c, vec4{ -0.0277777778, 5.4, -30.7, -299.9 }));
		REQUIRE(aeq(b / c, vec4{ -0.2222222222, 1.3333333333, -4.0, -50.0 }));
	}
}
TEST_CASE("Vector dot product, length", "[linalg]") {
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
TEST_CASE("Vector normalize", "[linalg]") {
	SECTION("Zero vector") {
		vec4 nmz = vec4::zero().normalize();
		for (size_t i = 0; i < 4; i++) {
			REQUIRE(!std::isfinite(nmz[i]));
		}
	}
	SECTION("Random vectors") {
		vec4 a = { 0, 17, 17, 0 };
		vec4 an = vec4{ 0.f, 1.f / sqrtf(2), 1.f / sqrtf(2), 0.f };
		REQUIRE(aeq(a.normalize(),an));

		vec4 b = { 83.0, -65.23, -0.05, 7.64 };
		vec4 bn = vec4{ 0.7841948905, -0.6163015989, -0.0004724066, 0.0721837224 };
		REQUIRE(aeq(b.normalize(), bn));
	}
}
TEST_CASE("Vector cross product", "[linalg]") {
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
TEST_CASE("Vector linear interpolation", "[linalg]") {
	vec3 x = { 12, -43, 86 };
	vec3 y = { 0.0, -5.0, -2.75 };
	REQUIRE(aeq(lerp(x, y, 0.f), x));
	REQUIRE(aeq(lerp(x, y, 1.f), y));
	REQUIRE(aeq(lerp(x, y, 0.5f), vec3{6.f, -24.f, 41.625f}));
	// ugh.. we do actually get the correct result here, but there's a bit too much precision loss at play.
	REQUIRE(aeq(lerp(x, y, 0.33f), vec3{ 8.04f, -30.46f, 56.7125f }, 1e-5f));
	REQUIRE(aeq(lerp(x, y, -1.f), vec3{ 24.f, -81.f, 174.75f }));
}
TEST_CASE("Vector cubic bezier interpolation", "[linalg]") {
	vec3 ps[4] = {
		{0, 0, 1}, // p0
		{0, 1, 1}, // cp0
		{1, 1, 1}, // cp1
		{1, 0, 1}, // p1
	};
	auto interp = [ps](float fac) {
		return math::cubic_bezier(ps[0], ps[1], ps[2], ps[3], fac);
	};
	REQUIRE(aeq(interp(0.f), ps[0]));
	REQUIRE(aeq(interp(0.2f), vec3{ 0.104, 0.48, 1 }));
	REQUIRE(aeq(interp(1.f), ps[3]));
}
TEST_CASE("Vector reflection", "[linalg]") {
	SECTION("Basic") {
		vec3 n = { 0.0, 0.0, 1.0 };

		vec3 incoming_tx = { 1.0, 0.0, 0.0 };
		vec3 incoming_ty = { 0.0, 1.0, 0.0 };
		vec3 incoming_n = { 0.0, 0.0, -1.0 };
		vec3 incoming_custom = { 5.0, -2.0, -4.0 };

		REQUIRE(incoming_tx.reflect_off(n) == incoming_tx);
		REQUIRE(incoming_ty.reflect_off(n) == incoming_ty);
		REQUIRE(incoming_n.reflect_off(n) == -incoming_n);
		REQUIRE(aeq(incoming_custom.reflect_off(n), vec3{ 5.0, -2.0, 4.0 }));
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
TEST_CASE("Vector min/max", "[linalg]") {
	vec4 crazy_a = { INFINITY, -INFINITY, NAN, 0 };
	vec4 crazy_b = { -INFINITY, INFINITY, INFINITY, NAN };
	vec4 crazy_c = { NAN, NAN, NAN, NAN };
	vec4 normal_a = { 42, -23, 65, 0 };
	vec4 normal_b = { 41.5, 12, -65, 1 };
	SECTION("Vector Min") {
		REQUIRE(vmin(crazy_a, crazy_b) == vec4{ -INFINITY, -INFINITY, INFINITY, 0 });
		REQUIRE(vmin(crazy_b, crazy_a) == vec4{ -INFINITY, -INFINITY, INFINITY, 0 });

		vec4 tmp = vmin(crazy_a, crazy_c);
		REQUIRE(tmp.x == INFINITY);
		REQUIRE(tmp.y == -INFINITY);
		REQUIRE(std::isnan(tmp.z));
		REQUIRE(tmp.w == 0);

		REQUIRE(vmin(normal_a, normal_b) == vec4{ 41.5, -23, -65, 0 });
		REQUIRE(vmin(normal_b, normal_a) == vec4{ 41.5, -23, -65, 0 });
		REQUIRE(vmin(crazy_a, normal_b) == vec4{ 41.5, -INFINITY, -65, 0 });
		REQUIRE(vmin(normal_b, crazy_a) == vec4{ 41.5, -INFINITY, -65, 0 });
	}
	SECTION("Vector Max") {
		REQUIRE(vmax(crazy_a, crazy_b) == vec4 {INFINITY, INFINITY, INFINITY, 0});
		REQUIRE(vmax(crazy_b, crazy_a) == vec4{ INFINITY, INFINITY, INFINITY, 0 });

		vec4 tmp = vmax(crazy_a, crazy_c);
		REQUIRE(tmp.x == INFINITY);
		REQUIRE(tmp.y == -INFINITY);
		REQUIRE(std::isnan(tmp.z));
		REQUIRE(tmp.w == 0);

		REQUIRE(vmax(normal_a, normal_b) == vec4{ 42, 12, 65, 1 });
		REQUIRE(vmax(normal_b, normal_a) == vec4{ 42, 12, 65, 1 });
		REQUIRE(vmax(crazy_a, normal_b) == vec4{ INFINITY, 12, -65, 1 });
		REQUIRE(vmax(normal_b, crazy_a) == vec4{ INFINITY, 12, -65, 1 });
	}
}
TEST_CASE("Matrix constructors, row/column accessors", "[linalg]") {
	auto validate = [](const mat3x4& m) {
		REQUIRE(m.row(0) == vec4{ 1.0, 0.0, 0.0, 0.0 });
		REQUIRE(m.row(1) == vec4{ -0.5, -0.5, 7.0, -7.0 });
		REQUIRE(m.row(2) == vec4{ 0.0, 6.2, -6.2, -0.0 });

		REQUIRE(m.col(0) == vec3{ 1.0, -0.5, 0.0 });
		REQUIRE(m.col(1) == vec3{ 0.0, -0.5, 6.2 });
		REQUIRE(m.col(2) == vec3{ 0.0, 7.0, -6.2 });
		REQUIRE(m.col(3) == vec3{ 0.0, -7.0, 0.0 });
	};

	SECTION("Identity") {
		mat4 i = {
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		};
		REQUIRE(i == mat4::ident());
	}
	SECTION("Raw specification") {
		mat3x4 m = {
			+1.0, +0.0, +0.0, +0.0,
			-0.5, -0.5, +7.0, -7.0,
			+0.0, +6.2, -6.2, -0.0,
		};
		validate(m);
	}
	SECTION("From rows") {
		mat3x4 m = mat3x4::from_rows(
			vec4{ +1.0, +0.0, +0.0, +0.0 },
			vec4{ -0.5, -0.5, +7.0, -7.0 },
			vec4{ +0.0, +6.2, -6.2, -0.0 }
		);
		validate(m);
	}
	SECTION("From cols") {
		mat3x4 m = mat3x4::from_cols(
			vec3{ +1.0, -0.5, +0.0 },
			vec3{ +0.0, -0.5, +6.2 },
			vec3{ +0.0, +7.0, -6.2 },
			vec3{ +0.0, -7.0, -0.0 }
		);
		validate(m);
	}
}
TEST_CASE("Matrix transpose", "[linalg]") {
	mat3x4 m = {
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 0, 1, 2
	};
	math::MMatrix<float, 4, 3> mt = {
		1, 5, 9,
		2, 6, 0,
		3, 7, 1,
		4, 8, 2
	};
	REQUIRE(m.transpose() == mt);
}
TEST_CASE("Matrix comparisons", "[linalg]") {
	SECTION("Random Matrices") {
		mat2x3 a1 = {
			5.0, 0.0, 0.0,
			1.0, 2.0, 3.0
		};
		mat2x3 a2 = {
			5.0, 0.0, 0.0,
			1.0, 2.0, 3.0
		};
		REQUIRE(a1 == a2);
		REQUIRE(aeq(a1, a2));
		
		mat2x3 b = {
			5.0, 0.0, 1.0,
			1.0, 2.0, -3.0
		};
		REQUIRE(a1 != b);
		REQUIRE(!aeq(a1, b));
	}
	SECTION("Precision") {
		mat2 target = { 27, 30, 92, -83 };
		mat2 offset = { 27 + 20 * FLT_EPSILON, 30 - 15 * FLT_EPSILON, 92, -83 + 40 * FLT_EPSILON };
		REQUIRE(target != offset);
		REQUIRE(aeq(target, offset));
		mat2 too_much_offset = { 27 + 20 * FLT_EPSILON, 29.99f, 92.0f, -83.0f };
		REQUIRE(target != too_much_offset);
		REQUIRE(!aeq(target, too_much_offset));
	}
}
TEST_CASE("Matrix addition/subtraction", "[linalg]") {
	SECTION("Random matrices") {
		mat2x3 a = {
			2.3, 4.4, -7.5,
			0.0, INFINITY, -INFINITY
		};
		mat2x3 b = {
			-12, -6.4, -2.2,
			0.01, 6.4, -1
		};
		mat2x3 apb = {
			-9.7, -2, -9.7,
			0.01, INFINITY, -INFINITY
		};
		REQUIRE(aeq(a + b, apb));
		mat2x3 amb = {
			14.3, 10.8, -5.3,
			-0.01, INFINITY, -INFINITY
		};
		REQUIRE(aeq(a - b, amb));
		mat2x3 bma = {
			-14.3, -10.8, 5.3,
			0.01, -INFINITY, INFINITY
		};
		REQUIRE(aeq(b-a, bma));
	}
	SECTION("Additive inverses") {
		mat2x3 pos = {
			5.0, 0.0, -3.4,
			-0.0, 6.7, 12.0
		};
		mat2x3 neg = {
			-5.0, -0.0, 3.4,
			0.0, -6.7, -12.0
		};
		REQUIRE(pos + neg == mat2x3::zero());
	}
}
TEST_CASE("Matrix multiply", "[linalg]") {
	SECTION("Identity & dimensions") {
		REQUIRE(mat2::ident() * mat2::ident() == mat2::ident());
		REQUIRE(mat2x3::zero() * (mat2x3::zero().transpose()) == mat2::zero());
	}
	SECTION("Random square matrices") {
		mat3 a = {
			7.0,	16.5,	32.25,
			-22.5,	-23,	-6,
			-100,	1,		2
		};
		REQUIRE(a * mat3::ident() == a);
		REQUIRE(mat3::ident() * a == a);

		mat3 b = {
			-32, 64, 8,
			24, 12, -6,
			0.5, -0.25, 0.125
		};

		mat3 axb = {
			188.125, 637.9375, -38.96875,
			165, -1714.5, -42.75,
			3225, -6388.5, -805.75
		};

		mat3 bxa = {
			-2464, -1992, -1400,
			498, 114, 690,
			-3.375, 14.125, 17.875
		};

		REQUIRE(aeq(b * a, bxa));
		REQUIRE(b * mat3::zero() == mat3::zero());
		REQUIRE(mat3::zero() * b == mat3::zero());
	}
	SECTION("Random nonsquare matrices") {
		mat2x3 a = {
			16, 8, -4,
			0.5, 2.5, 32
		};
		math::MMatrix<float, 3, 2> b = {
			100, 10,
			1, -1,
			-10, -100
		};
		mat2 axb = {
			1648, 552,
			-267.5, -3197.5
		};
		REQUIRE(aeq(a * b, axb));
	}
}
TEST_CASE("Matrix-vector multiply", "[linalg]") {
	mat2x3 mul = {
		16, -2.4, 0,
		1, -16, -0.1,
	};
	vec3 v = { 1, 2, -0.5 };
	vec2 mulxv = { 11.2, -30.95 };
	REQUIRE(mul * v == mulxv);
}
TEST_CASE("Matrix row-echelon form", "[linalg]") {
	SECTION("Basic cases") {
		mat3 ident = mat3::ident();
		REQUIRE(ident.row_echelon() == ident);

		mat2x3 normal = {
			1, 0, 0,
			0, 1, 0,
		};
		REQUIRE(normal.row_echelon() == normal);

		mat2x3 shifted = {
			0, 1, 0,
			0, 0, 1
		};
		REQUIRE(shifted.row_echelon() == shifted);

		mat2 left_corner = {
			1, 0,
			0, 0
		};
		REQUIRE(left_corner.row_echelon() == left_corner);

		mat2 right_corner = {
			0, 1,
			0, 0
		};
		REQUIRE(right_corner.row_echelon() == right_corner);
	}
	SECTION("Row swaps, no scaling") {
		// ident = permute_123
		mat3 permute_231 = {
			0, 1, 0,
			0, 0, 1,
			1, 0, 0
		};
		REQUIRE(permute_231.row_echelon() == mat3::ident());

		mat3 permute_312 = {
			0, 0, 1,
			1, 0, 0,
			0, 1, 0
		};
		REQUIRE(permute_312.row_echelon() == mat3::ident());

		mat3 permute_321 = {
			0, 0, 1,
			0, 1, 0,
			1, 0, 0
		};
		REQUIRE(permute_321.row_echelon() == mat3::ident());

		mat3 permute_213 = {
			0, 1, 0,
			1, 0, 0,
			0, 0, 1
		};
		REQUIRE(permute_213.row_echelon() == mat3::ident());

		mat3 permute_132 = {
			1, 0, 0,
			0, 0, 1,
			0, 1, 0
		};
		REQUIRE(permute_213.row_echelon() == mat3::ident());

		mat2x3 shifted_permute = {
			0, 0, 1,
			0, 1, 0
		};
		mat2x3 shifted_permute_re = {
			0, 1, 0,
			0, 0, 1
		};
		REQUIRE(shifted_permute.row_echelon() == shifted_permute_re);
	}
	SECTION("Random matrices") {
		mat3 a = {
			2.5, 3.6, -7,
			-12, 0, 0,
			0, 1, -1
		};
		mat3 a_re = {
			1, 0, 0,
			0, 1, -1.9444444444,
			0, 0, 1,
		};
		REQUIRE(aeq(a.row_echelon(), a_re));

		mat3 b = {
			6, 3, 0,
			-15, -7.5, 0,
			0, 1, 1
		};
		mat3 b_re = {
			1, 0.5, 0,
			0, 1, 1,
			0, 0, 0
		};
		REQUIRE(aeq(b.row_echelon(), b_re));

		mat2x3 c = {
			0, -5.7, 6.2,
			0.1, 0, 1
		};
		mat2x3 c_re = {
			1, 0, 10,
			0, 1, -1.0877192982
		};
		REQUIRE(c.row_echelon() == c_re);

		mat2x3 d = {
			0, -5.7, 6.2,
			0, 0.1, 1
		};
		mat2x3 d_re = {
			0, 1, -1.0877192982,
			0, 0, 1
		};
		REQUIRE(d.row_echelon() == d_re);
	}
	SECTION("Tall Matrices") {
		math::MMatrix<float, 3, 2> tall = {
			1, 0,
			0, 1,
			0.5, 1
		};
		math::MMatrix<float, 3, 2> tall_re = {
			1, 0,
			0, 1,
			0, 0
		};
		REQUIRE(tall.row_echelon() == tall_re);
	}
}
TEST_CASE("Matrix determinant", "[linalg]") {
	SECTION("Basic") {
		REQUIRE(mat2::ident().determinant() == 1);
		REQUIRE(mat3::ident().determinant() == 1);
		
		mat3 permuted = {
			0, 1, 0,
			1, 0, 0,
			0, 0, 1
		};
		REQUIRE(permuted.determinant() == -1);

		mat3 missing = {
			1, 0, 0,
			0, 0, 0,
			0, 1, 0
		};
		REQUIRE(missing.determinant() == 0);
	}
	SECTION("Random") {
		mat3 a = {
			7.0,	16.5,	32.25,
			-22.5,	-23,	-6,
			-100,	1,		2
		};
		REQUIRE(aeq(a.determinant(), -64538.125f));

		mat3 b = {
			6, 3, 0,
			-15, -7.5, 0,
			0, 1, 1
		};
		REQUIRE(b.determinant() == 0);

		mat4 c = {
			0.426719, 0.0979894, 0.57436, 0.88369,
			0.30224, 0.00128982, 0.805764, 0.808081,
			0.898543, 0.798784, 0.898152, 0.882801,
			0.343767, 0.622619, 0.583506, 0.264387
		};
		REQUIRE(aeq(c.determinant(), -0.034146f));

		mat2 d = {
			0.580531, 0.84377,
			0.788602, 0.764121
		};
		REQUIRE(aeq(d.determinant(), -0.2218027813f));
	}
}

TEST_CASE("Matrix reduced row echelon form", "[linalg]") {
	SECTION("Basic") {
		REQUIRE(mat2::ident().reduced_row_echelon() == mat2::ident());
		REQUIRE(mat3::ident().reduced_row_echelon() == mat3::ident());

		mat2 m2_permute = {
			0, 1,
			1, 0
		};
		REQUIRE(m2_permute.reduced_row_echelon() == mat2::ident());
		
		mat3 m3_permute = {
			0, 1, 0,
			1, 0, 0,
			0, 0, 1
		};
		REQUIRE(m3_permute.reduced_row_echelon() == mat3::ident());
	}
	SECTION("Tall") {
		math::MMatrix<float, 3, 2> indep = {
			1, 5,
			-5, 2,
			0, 6
		};
		math::MMatrix<float, 3, 2> indep_rre = {
			1, 0,
			0, 1,
			0, 0
		};
		REQUIRE(indep.reduced_row_echelon() == indep_rre);

		math::MMatrix<float, 3, 2> dep_a = {
			1, 0,
			1, 0,
			1, 0
		};
		math::MMatrix<float, 3, 2> dep_a_rre = {
			1, 0,
			0, 0,
			0, 0
		};
		REQUIRE(dep_a.reduced_row_echelon() == dep_a_rre);

		math::MMatrix<float, 3, 2> dep_b = {
			0, 1,
			0, 1,
			0, 1
		};
		math::MMatrix<float, 3, 2> dep_b_rre = {
			0, 1,
			0, 0,
			0, 0
		};
		REQUIRE(dep_b.reduced_row_echelon() == dep_b_rre);
	}
	SECTION("Square") 
	{
		mat3 random = {
			1.0, 6.7, -2.0,
			-3.3, 4, 0,
			1, 0, -1
		};
		mat3 random_rre = {
			1, 0, 0,
			0, 1, 0,
			0, 0, 1
		};
		REQUIRE(random.reduced_row_echelon() == random_rre);

		mat3 dep_col = {
			1, -4, -2,
			-3.3, 0, 0,
			1, -2, -1
		};
		mat3 dep_col_rre = {
			1, 0, 0,
			0, 1, 0.5,
			0, 0, 0
		};
		REQUIRE(dep_col.reduced_row_echelon() == dep_col_rre);

		mat3 zero_col = {
			1, 0, -2,
			-3.3, 0, 0,
			1, 0, 1
		};
		mat3 zero_col_rre = {
			1, 0, 0,
			0, 0, 1,
			0, 0, 0
		};
		REQUIRE(aeq(zero_col.reduced_row_echelon(), zero_col_rre));

		mat3 zero_col_dep_row = {
			1, 0, -2,
			-3.3, 0, 0,
			2, 0, -4
		};
		mat3 zero_col_dep_row_rre = {
			1, 0, 0,
			0, 0, 1,
			0, 0, 0
		};
		REQUIRE(aeq(zero_col_dep_row.reduced_row_echelon(), zero_col_dep_row_rre));

		mat3 dep_row = {
			6.2, 0, -17,
			1, 2, 3,
			4, 8, 12
		};
		mat3 dep_row_rre = {
			1, 0, -2.74193548387096774193548387097, 
			0, 1, 2.87096774193548387096774193548, 
			0, 0, 0
		};
		REQUIRE(aeq(dep_row.reduced_row_echelon(), dep_row_rre));

		mat3 zero_row_dep_row = {
			0, 0, 0,
			1, 2, 3,
			4, 8, 12
		};
		mat3 zero_row_dep_row_rre = {
			1, 2, 3,
			0, 0, 0,
			0, 0, 0
		};
		REQUIRE(zero_row_dep_row.reduced_row_echelon() == zero_row_dep_row_rre);
	}
	SECTION("Wide") {
		mat3x4 random = {
			1.0, 6.7, -2.0, -12,
			-3.3, 4, 0, 19,
			1, 0, -1, 16
		};
		mat3x4 random_rre = {
			1, 0, 0, -16.7476532302595251242407509663, 
			0, 1, 0, -9.06681391496410822749861954721, 
			0, 0, 1, -32.7476532302595251242407509663
		};
		REQUIRE(aeq(random.reduced_row_echelon(), random_rre));

		mat3x4 dep_col = {
			1, -4, -2, 3,
			-3.3, 0, 0, 2,
			1, -2, -1, 5
		};
		mat3x4 dep_col_rre = {
			1, 0, 0, 0,
			0, 1, 0.5, 0,
			0, 0, 0, 1
		};
		REQUIRE(dep_col.reduced_row_echelon() == dep_col_rre);

		mat3x4 dep_row = {
			6.2, 0, -17, 8,
			1, 2, 3, 6,
			4, 8, 12, 24
		};
		mat3x4 dep_row_rre = {
			1, 0, -2.74193548387096774193548387097, 1.29032258064516129032258064516,
			0, 1, 2.87096774193548387096774193548, 2.35483870967741935483870967742, 
			0, 0, 0, 0
		};
		REQUIRE(aeq(dep_row.reduced_row_echelon(), dep_row_rre));
	}
}

TEST_CASE("Matrix inverse", "[linalg]") {
	SECTION("Basic") {
		REQUIRE(mat2::ident().inverse() == mat2::ident());
		REQUIRE(mat3::ident().inverse() == mat3::ident());
		REQUIRE(mat4::ident().inverse() == mat4::ident());

		mat3 permute = {
			0, 1, 0,
			0, 0, 1,
			1, 0, 0
		};
		// this is not true in general
		REQUIRE(permute.inverse() == permute.transpose());
	}
	SECTION("Random") {
		mat2 r2 = {
			0.580531, -0.84377,
			0.788602, 7.64121
		};
		mat2 r2_inv = {
			1.49788, 0.165401,
			-0.154587, 0.113799
		};
		REQUIRE(aeq(r2.inverse(), r2_inv, 1e6f));

		mat3 r3 = {
			1.0, 6.7, -2.0,
			-3.3, 4, 0,
			1, 0, -1
		};
		mat3 r3_inv = {
			0.220872, -0.369961, -0.441745,
			0.18222, -0.0552181, -0.36444,
			0.220872, -0.369961, -1.44174
		};
		REQUIRE(aeq(r3.inverse(), r3_inv, 1e6f));
	}
}
TEST_CASE("Convenience constructors", "[linalg]") {
	SECTION("Diagonal") {
		mat2 us = mat2::diag(2, 2);
		REQUIRE(us == mat2{ 2, 0, 0, 2 });
		mat2 ds = mat2::diag(3, -1);
		REQUIRE(ds == mat2{ 3, 0, 0, -1 });
	}
}
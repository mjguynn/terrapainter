#pragma once
#include <algorithm>
#include <cstdlib>
#include <concepts>
#include <cmath>
#include <limits>
#include <iostream>

#include "util.h"

//! \file linalg.h
//! Graphics-oriented linear algebra.

namespace math {
	// For the sake of programmer sanity and debug-mode performance,
	// we use union type punning,  which is technically illegal C++
	// but is accepted by the Big Three compilers since literally
	// everyone uses it.

	template<std::floating_point F, size_t C>
	struct alignas(sizeof(F) * 4) MVectorMembers {
		F x;
		F y;
		F z;
		F w;

		template<std::convertible_to<F> S>
		MVectorMembers(S x, S y, S z, S w) : x(x), y(y), z(z), w(w) {}
	};

	template<std::floating_point F>
	struct MVectorMembers<F, 2> {
		F x;
		F y;

		template<std::convertible_to<F> S>
		MVectorMembers(S x, S y) : x(x), y(y) {}
	};

	template<std::floating_point F>
	struct alignas(sizeof(F) * 4) MVectorMembers<F, 3> {
		F x;
		F y;
		F z;

		template<std::convertible_to<F> S>
		MVectorMembers(S x, S y, S z) : x(x), y(y), z(z) {}
	};

	template<std::floating_point F, size_t C>
		requires(1 <= C && C <= 4)
	union MVector {
		/// The elements of the vector exposed as a simple array.
		/// This allows operations like looping over the components
		/// in a for-loop.
		F elems[C];

		/// The elements of the vector exposed as named fields.
		/// To get the Y fields, you'd do `v.f.x`.
		/// This is named `f` for no particular reason other than
		/// being super short and easy to type.
		MVectorMembers<F, C> f;

		/// The number of elements in vectors of this type.
		constexpr static size_t SIZE = C;

		/// Scalar "splat" scalar cast. Sets all elements to `splat`.
		template<std::convertible_to<F> S>
		explicit constexpr MVector(S splat) {
			for (size_t i = 0; i < C; i++) this->elems[i] = splat;
		}

		template<std::convertible_to<F> S>
		constexpr static MVector splat(S splat) {
			return static_cast<MVector>(splat);
		}

		/// Per-element bracket initialization syntax.
		/// You have to specify the same number of elements as
		/// the vector requires.
		template<typename ...Args>
		constexpr MVector(const Args&... args) : f(args...) {}

		constexpr bool operator==(const MVector& other) const {
			return aeq(*this, other);
		}
		constexpr bool operator!=(const MVector& other) const {
			return !this->operator==(other);
		}

		constexpr MVector& operator+=(const MVector& other) {
			for (size_t i = 0; i < C; i++) this->elems[i] += other.elems[i];
			return *this;
		}
		constexpr MVector operator+(const MVector& other) const {
			auto copy = *this;
			copy += other;
			return copy;
		}

		constexpr MVector& operator-=(const MVector& other) {
			for (size_t i = 0; i < C; i++) this->elems[i] -= other.elems[i];
			return *this;
		}
		constexpr MVector operator-(const MVector& other) const {
			auto copy = *this;
			copy -= other;
			return copy;
		}

		/// Scalar multiplication.
		template<std::convertible_to<F> S>
		constexpr MVector& operator*=(S scalar) {
			F scalar_cvt = static_cast<F>(scalar);
			for (size_t i = 0; i < C; i++) this->elems[i] *= scalar_cvt;
			return *this;
		}
		template<std::convertible_to<F> S>
		constexpr MVector operator*(S scalar) const {
			auto copy = *this;
			copy *= scalar;
			return copy;
		}

		/// Element-wise vector multiplication (not the dot product!)
		constexpr MVector& operator*=(const MVector& other) {
			for (size_t i = 0; i < C; i++) this->elems[i] *= other.elems[i];
			return *this;
		}
		constexpr MVector operator*(const MVector& other) const {
			auto copy = *this;
			copy *= other;
			return copy;
		}

		/// Scalar division.
		template<std::convertible_to<F> S>
		constexpr MVector& operator/=(S scalar) {
			// Find reciprocal and multiply by that,
			// since float division is super slow.
			F reciprocal = static_cast<F>(1.0) / static_cast<F>(scalar);
			return this->operator*=(reciprocal);
		}
		template<std::convertible_to<F> S>
		constexpr MVector operator/(S scalar) const {
			auto copy = *this;
			copy /= scalar;
			return copy;
		}

		/// Element-wise vector division.
		constexpr MVector& operator/=(const MVector& other) {
			for (size_t i = 0; i < C; i++) this->elems[i] /= other.elems[i];
			return *this;
		}
		constexpr MVector operator/(const MVector& other) const {
			auto copy = *this;
			copy /= other;
			return copy;
		}

		/// Indexed element access.
		constexpr const F& operator[](size_t index) const {
			return this->elems[index];
		}
		constexpr F& operator[](size_t index) {
			return this->elems[index];
		}

		// Returns the magnitude of this vector.
		constexpr F mag() const {
			return std::sqrt(dot(*this, *this));
		}
	};

	template<std::floating_point F, size_t C>
	std::ostream& operator<<(std::ostream& os, const MVector<F,C>& vec)
	{
		os << "(" << vec[0]; 
		for (size_t i = 1; i < C; i++) os << ", " << vec[i];
		os << ")";
		return os;
	}

	template<std::floating_point F, size_t C, std::convertible_to<F> S>
	MVector<F,C> operator*(S scalar, const MVector<F, C>& vec) {
		return vec * scalar;
	}


	template<std::floating_point F, size_t C>
	F dot(const MVector<F, C>& left, const MVector<F, C>& right) {
		F sum = static_cast<F>(0);
		for (size_t i = 0; i < C; i++) sum += left.elems[i] * right.elems[i];
		return sum;
	}

	template<std::floating_point F, size_t C, std::convertible_to<F> S>
	F lerp(const MVector<F, C>& p0, const MVector<F, C>& p1, S factor) {
		F factor_cvt = static_cast<F>(factor);
		return (static_cast<F>(1) - factor_cvt) * p0 + (factor_cvt) * p1;
	}

	template<std::floating_point F, size_t C, std::convertible_to<F> S>
	F cerp(const MVector<F, C>& p0, const MVector<F, C>& cp0, const MVector<F, C>& cp1, const MVector<F, C>& p1, S factor) {
		F factor_cvt = static_cast<F>(factor);
		// Waiting to implement cubic interpolation until I've done matrices
		TODO();
	}

	template<std::floating_point F>
	bool aeq(F left, F right, F tolerance = std::numeric_limits<F>::epsilon()) {
		if (!std::isfinite(left)) {
			return !std::isfinite(right) && left == right;
		}
		else if (!std::isfinite(right)) {
			return false;
		}
		F lowmag = std::min(std::abs(left), std::abs(right));
		F scale = std::max(lowmag, static_cast<F>(1));
		return std::abs(left - right) <= (tolerance * scale);
	}

	template<std::floating_point F, size_t C>
	bool aeq(const MVector<F, C>& left, const MVector<F, C>& right, F tolerance = std::numeric_limits<F>::epsilon()) {
		for (size_t i = 0; i < C; i++) {
			if (!aeq(left.elems[i], right.elems[i], tolerance)) return false;
		}
		return true;
	}

}

using vec2 = math::MVector<float, 2>;
using vec3 = math::MVector<float, 3>;
using vec4 = math::MVector<float, 4>;

using math::dot;
using math::aeq;

#pragma once
#include <algorithm>
#include <array>
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
	// everyone uses it. Same deal with anonymous structs/unions.

	template<std::floating_point F, size_t C>
	struct alignas(sizeof(F) * 4) MVectorStorage {
		union {
			std::array<F, C> elems;
			struct { F x, y, z, w; };
		};
	};
	template<std::floating_point F>
	struct alignas(sizeof(F) * 4) MVectorStorage<F, 3> {
		union {
			std::array<F, 3> elems;
			struct { F x, y, z; };
		};
	};
	template<std::floating_point F>
	struct MVectorStorage<F, 2> {
		union {
			std::array<F, 2> elems;
			struct { F x, y; };
		};
	};

	template<std::floating_point F, size_t C>
		requires(2 <= C && C <= 4)
	struct MVector : public MVectorStorage<F, C> {
		/// The number of elements in vectors of this type.
		constexpr static size_t SIZE = C;

		/// Scalar "splat" scalar cast. Sets all elements to `splat`.
		template<std::convertible_to<F> S>
		explicit constexpr MVector(S splat) : MVectorStorage<F, C> {} {
			this->elems.fill(static_cast<F>(splat));
		}

		template<std::convertible_to<F> S>
		constexpr static MVector splat(S splat) {
			return static_cast<MVector>(splat);
		}

		consteval static MVector zero() {
			return MVector::splat(0);
		}

		/// Per-element bracket initialization syntax.
		template<typename ...Args>
			requires(sizeof...(Args) == C && std::conjunction_v<std::is_convertible<F, Args>...>)
		constexpr MVector(const Args&... args) : MVectorStorage<F,C> { static_cast<F>(args)... } {}

		constexpr bool operator==(const MVector& other) const {
			return aeq(*this, other);
		}
		constexpr bool operator!=(const MVector& other) const = default;

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

		/// Unary negation.
		constexpr MVector operator-() const {
			return *this * static_cast<F>(-1);
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
			// If we ever extend this class to int vecs, this needs to change
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

		constexpr MVector normalize() const {
			return *this / this->mag();
		}

		// Assumes that n is normalized.
		constexpr MVector reflect_off(const MVector& normal) const {
			// Intellisense syntax highlighting *HATES* this line for some reason
			MVector normal_component = dot(*this, normal) * normal;
			return *this - (2 * normal_component);
		}
	};

	template<std::floating_point F, size_t C>
	inline std::ostream& operator<<(std::ostream& os, const MVector<F,C>& vec)
	{
		os << "(" << vec[0]; 
		for (size_t i = 1; i < C; i++) os << ", " << vec[i];
		os << ")";
		return os;
	}

	template<std::floating_point F, size_t C, std::convertible_to<F> S>
	inline constexpr MVector<F,C> operator*(S scalar, const MVector<F, C>& vec) {
		return vec * scalar;
	}


	template<std::floating_point F, size_t C>
	inline constexpr F dot(const MVector<F, C>& l, const MVector<F, C>& r) {
		F sum = static_cast<F>(0);
		for (size_t i = 0; i < C; i++) sum += l.elems[i] * r.elems[i];
		return sum;
	}

	template<std::floating_point F, size_t C, std::convertible_to<F> S>
	inline constexpr MVector<F,C> lerp(const MVector<F, C>& p0, const MVector<F, C>& p1, S factor) {
		F factor_cvt = static_cast<F>(factor);
		return (static_cast<F>(1) - factor_cvt) * p0 + (factor_cvt) * p1;
	}

	template<std::floating_point F, size_t C, std::convertible_to<F> S>
	inline constexpr MVector<F,C> cerp(const MVector<F, C>& p0, const MVector<F, C>& cp0, const MVector<F, C>& cp1, const MVector<F, C>& p1, S factor) {
		F factor_cvt = static_cast<F>(factor);
		// Waiting to implement cubic interpolation until I've done matrices
		TODO();
	}

	template<std::floating_point F>
	inline constexpr MVector<F, 3> cross(const MVector<F, 3>& a, const MVector<F, 3>& b) {
		return MVector<F, 3> {
			a.y* b.z - a.z * b.y,
			a.z* b.x - a.x * b.z,
			a.x* b.y - a.y * b.x
		};
	}

	// TODO: vector max/min (elementwise)

	template<std::floating_point F, size_t M, size_t N>
		requires (2 <= M <= 4 && 2 <= N <= 4)
	class MMatrix {
		// *currently* we store column vectors instead of row vectors
		// My microoptimization focused reasoning is that matrix*matrix
		// multiplication is left-associative in C++ and we'll do a lot
		// more of that than matrix * vector multiplication, and there's
		// slightly less weird swizzling if you use column vectors
		// ... this could change at any time though
		MVector<F, M> cols[N];
		
		
		template<typename... Args>
		constexpr MMatrix(const Args&... cols) : cols{ cols... } {}

	public:
		template<typename... Args>
			requires(sizeof...(Args) == N * M)
		constexpr MMatrix(const Args&... args) {
			TODO();
		}
		constexpr MVector<F, M> col(size_t i) const {
			return cols[i];
		}
		constexpr MVector<F, N> row(size_t j) const {
			auto rv = MVector<F, N>::zero();
			for (size_t i = 0; i < N; i++) rv[i] = cols[i][j];
			return rv;
		}

		// Just hammering out the API
		template<std::convertible_to<F> S>
		consteval static MMatrix splat(S splat) {
			MMatrix z;
			for (size_t i = 0; i < M; i++) {
				z.cols[i] = MVector<F,M>::splat(splat);
			}
			return z;
		}

		consteval static MMatrix zero() {
			return MMatrix::splat(static_cast<F>(0));
		}

		consteval static MMatrix identity() requires (N==M) {
			auto mat = MMatrix::zero();
			for (size_t i = 0; i < N; i++) {
				mat[i][i] = static_cast<F>(1);
			}
			return mat;
		}

		template<typename... Args>
			requires(sizeof...(Args) == M && std::conjunction_v<std::is_same<MVector<F,M>, Args>...>)
		constexpr static MMatrix from_rows(const Args&... rows) {
			auto mat = MMatrix::zero();
			TODO();
		}

		template<typename... Args>
			requires(sizeof...(Args) == N && std::conjunction_v<std::is_same<MVector<F, N>, Args>...>)
		constexpr static MMatrix from_cols(const Args&... cols) {
			return MMatrix(cols...);
		}

		// We DON'T do rotate, scale, transform matrices here
		// rotate: only makes sense for square & different in each dim
		// scale: only makes sense for square matrices
		// transform: should be in MMatrixH subclass
	};


	template<std::floating_point F>
	inline bool aeq(F l, F r, F tolerance = std::numeric_limits<F>::epsilon()) {
		if (!std::isfinite(l)) {
			return !std::isfinite(r) && l == r;
		}
		else if (!std::isfinite(r)) {
			return false;
		}
		F lowmag = std::min(std::abs(l), std::abs(r));
		F scale = std::max(lowmag, static_cast<F>(1));
		return std::abs(l - r) <= (tolerance * scale);
	}

	template<std::floating_point F, size_t C>
	inline bool aeq(const MVector<F, C>& left, const MVector<F, C>& right, F tolerance = std::numeric_limits<F>::epsilon()) {
		for (size_t i = 0; i < C; i++) {
			if (!aeq(left.elems[i], right.elems[i], tolerance)) return false;
		}
		return true;
	}
}

using vec2 = math::MVector<float, 2>;
using vec3 = math::MVector<float, 3>;
using vec4 = math::MVector<float, 4>;

using mat2 = math::MMatrix<float, 2, 2>;
using mat3 = math::MMatrix<float, 3, 3>;
using mat4 = math::MMatrix<float, 4, 4>;

using math::dot;
using math::cross;
using math::aeq;

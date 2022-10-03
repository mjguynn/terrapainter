﻿#pragma once
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

	// Operator access relies on reinterpret_cast, which does not function
	// in a constant context.
	#define _IMPL_CONSTEVAL_ACCESS(idx, x, y, z, w, rest) \
		if(std::is_constant_evaluated()) { \
			switch (idx) { \
				case 0: return x; \
				case 1: return y; \
				case 2: return z; \
				case 3: return w; \
				default: return rest; \
			} \
		} \
	
	#define IMPL_MEMBER_ACCESS(count, x, y, z, w, rest) \
		constexpr F& operator[](size_t idx) { \
			check_member_layout(); \
			_IMPL_CONSTEVAL_ACCESS(idx, x, y, z, w, rest); \
			assert(idx < count); \
			char* member = reinterpret_cast<char*>(this) + sizeof(F) * idx; \
			return *reinterpret_cast<F*>(member); \
		} \
		constexpr const F& operator[](size_t idx) const { \
			check_member_layout(); \
			_IMPL_CONSTEVAL_ACCESS(idx, x, y, z, w, rest); \
			assert(idx < count); \
			const char* member = reinterpret_cast<const char*>(this) + sizeof(F) * idx; \
			return *reinterpret_cast<const F*>(member); \
		}

	template<std::floating_point F, size_t C>
	struct MVectorStorage {
		F x, y, z, w;
		F mExtra[C-4];
		IMPL_MEMBER_ACCESS(C, x, y, z, w, mExtra[idx - 4])

		constexpr MVectorStorage() = default;

		template<typename ...Args>
			requires(sizeof...(Args) == C-4 && std::conjunction_v<std::is_same<F, Args>...>)
		constexpr MVectorStorage(F _x, F _y, F _z, F _w, Args... _extra)
			: x(_x), y(_y), z(_z), w(_w), mExtra{ _extra... } {}
	private:
		// These need to be in a function so they can refer to MVectorStorage.
		consteval static void check_member_layout() {
			static_assert(std::is_standard_layout<MVectorStorage>());
			static_assert(offsetof(MVectorStorage, x) == 0 * sizeof(F));
			static_assert(offsetof(MVectorStorage, y) == 1 * sizeof(F));
			static_assert(offsetof(MVectorStorage, z) == 2 * sizeof(F));
			static_assert(offsetof(MVectorStorage, w) == 3 * sizeof(F));
			static_assert(offsetof(MVectorStorage, mExtra) == 4 * sizeof(F));
		}
	};

	template<std::floating_point F>
	struct alignas(sizeof(F) * 4) MVectorStorage<F, 4> {
		F x, y, z, w;
		IMPL_MEMBER_ACCESS(4, x, y, z, w, w);
	private:
		// These need to be in a function so they can refer to MVectorStorage.
		consteval static void check_member_layout() {
			static_assert(std::is_standard_layout<MVectorStorage>());
			static_assert(offsetof(MVectorStorage, x) == 0 * sizeof(F));
			static_assert(offsetof(MVectorStorage, y) == 1 * sizeof(F));
			static_assert(offsetof(MVectorStorage, z) == 2 * sizeof(F));
			static_assert(offsetof(MVectorStorage, w) == 3 * sizeof(F));
		}
	};

	template<std::floating_point F>
	struct alignas(sizeof(F) * 4) MVectorStorage<F, 3> {
		F x, y, z;
		IMPL_MEMBER_ACCESS(3, x, y, z, z, z);
	private:
		// These need to be in a function so they can refer to MVectorStorage.
		consteval static void check_member_layout() {
			static_assert(std::is_standard_layout<MVectorStorage>());
			static_assert(offsetof(MVectorStorage, x) == 0 * sizeof(F));
			static_assert(offsetof(MVectorStorage, y) == 1 * sizeof(F));
			static_assert(offsetof(MVectorStorage, z) == 2 * sizeof(F));
		}
	};
	template<std::floating_point F>
	struct MVectorStorage<F, 2> {
		F x, y;
		IMPL_MEMBER_ACCESS(2, x, y, y, y, y);
	private:
		// These need to be in a function so they can refer to MVectorStorage.
		consteval static void check_member_layout() {
			static_assert(std::is_standard_layout<MVectorStorage>());
			static_assert(offsetof(MVectorStorage, x) == 0 * sizeof(F));
			static_assert(offsetof(MVectorStorage, y) == 1 * sizeof(F));
		}
	};

	#undef IMPL_MEMBER_ACCESS
	#undef _IMPL_CONSTEVAL_ACCESS

	template<std::floating_point F, size_t C> 
		requires(2 <= C)
	struct MVector : public MVectorStorage<F, C> {
		/// The number of elements in vectors of this type.
		constexpr static size_t SIZE = C;

		/// Default constructor
		constexpr MVector() : MVectorStorage<F, C>{} {}

		/// Scalar "splat" scalar cast. Sets all elements to `splat`.
		template<std::convertible_to<F> S>
		explicit constexpr MVector(S splat) : MVector() {
			F converted = static_cast<F>(splat);
			for (size_t i = 0; i < C; i++) {
				(*this)[i] = converted;
			}
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
		constexpr MVector(const Args&... args) : MVectorStorage<F,C> ( static_cast<F>(args)... ) {}

		constexpr bool operator==(const MVector& other) const {
			for(size_t i = 0; i < C; i++){
				if ((*this)[i] != other[i]) return false;
			}
			return true;
		}

		constexpr bool operator!=(const MVector& other) const { return !(*this == other); }

		#define VEC_DERIVE_BINOP(sym, in) \
		constexpr MVector operator sym(in other) const { \
			 auto copy = *this; copy sym##= other; return copy; \
		}

		constexpr MVector& operator+=(const MVector& other) {
			for (size_t i = 0; i < C; i++) (*this)[i] += other[i];
			return *this;
		}
		VEC_DERIVE_BINOP(+, const MVector&);
		

		constexpr MVector& operator-=(const MVector& other) {
			for (size_t i = 0; i < C; i++) (*this)[i] -= other[i];
			return *this;
		}
		VEC_DERIVE_BINOP(-, const MVector&);

		/// Unary negation.
		constexpr MVector operator-() const {
			return *this * static_cast<F>(-1);
		}

		/// Scalar multiplication.
		template<std::convertible_to<F> S>
		constexpr MVector& operator*=(S scalar) {
			F scalar_cvt = static_cast<F>(scalar);
			for (size_t i = 0; i < C; i++) (*this)[i] *= scalar_cvt;
			return *this;
		}
		template<std::convertible_to<F> S>
		VEC_DERIVE_BINOP(*, S);

		/// Element-wise vector multiplication (not the dot product!)
		constexpr MVector& operator*=(const MVector& other) {
			for (size_t i = 0; i < C; i++) (*this)[i] *= other[i];
			return *this;
		}
		VEC_DERIVE_BINOP(*, const MVector&);

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
		VEC_DERIVE_BINOP(/, S)

		/// Element-wise vector division.
		constexpr MVector& operator/=(const MVector& other) {
			for (size_t i = 0; i < C; i++) (*this)[i] /= other[i];
			return *this;
		}
		VEC_DERIVE_BINOP(/, const MVector&)

		#undef VEC_DERIVE_BINOP

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

		template<size_t Start, size_t End>
			requires(Start + 1 < End && End <= C)
		constexpr MVector<F, End - Start> slice() const {
			auto sliced = MVector<F, End - Start>::zero();
			for (size_t i = 0; i < End - Start; i++) {
				sliced[i] = (*this)[i+Start];
			}
			return sliced;
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
		for (size_t i = 0; i < C; i++) sum += l[i] * r[i];
		return sum;
	}

	template<std::floating_point F, size_t C, std::convertible_to<F> S>
	inline constexpr MVector<F,C> lerp(const MVector<F, C>& p0, const MVector<F, C>& p1, S factor) {
		F factor_cvt = static_cast<F>(factor);
		return (static_cast<F>(1) - factor_cvt) * p0 + (factor_cvt) * p1;
	}

	template<std::floating_point F>
	inline constexpr MVector<F, 3> cross(const MVector<F, 3>& a, const MVector<F, 3>& b) {
		return MVector<F, 3> {
			a.y* b.z - a.z * b.y,
			a.z* b.x - a.x * b.z,
			a.x* b.y - a.y * b.x
		};
	}

	template<std::floating_point F, size_t C>
	inline MVector<F, C> vmax(const MVector<F, C>& left, const MVector<F, C>& right) {
		auto out = MVector<F, C>::zero();
		for (size_t i = 0; i < C; i++) out[i] = std::fmax(left[i], right[i]);
		return out;
	}

	template<std::floating_point F, size_t C>
	inline MVector<F, C> vmin(const MVector<F, C>& left, const MVector<F, C>& right) {
		auto out = MVector<F, C>::zero();
		for (size_t i = 0; i < C; i++) out[i] = std::fmin(left[i], right[i]);
		return out;
	}

	template<std::floating_point F, size_t M, size_t N>
		requires (2 <= M && 2 <= N)
	class MMatrix {
		// Row-major storage.
		std::array<MVector<F, N>, M> mStorage;

	public:
		/// Default constructor (all zeroes).
		constexpr MMatrix() : mStorage() {}

		/// Entry-by-entry initialization.
		template<typename... Args>
			requires(sizeof...(Args) == N * M && std::conjunction_v<std::is_convertible<F, Args>...>)
		constexpr MMatrix(const Args&... args) : MMatrix() {
			// This initialization code is absolutely batshit insane
			auto write_entry = [](MMatrix& mat, size_t& k, F val) {
				mat.mStorage[k / N][k % N] = val;
				++k;
			};
			size_t i = 0;
			(..., write_entry(*this, i, static_cast<F>(args)));
		}

		constexpr MVector<F, M> col(size_t j) const {
			auto rv = MVector<F, M>::zero();
			for (size_t i = 0; i < M; i++) rv[i] = mStorage[i][j];
			return rv;
		}

		constexpr MMatrix& set_col(size_t j, const MVector<F, M>& c) {
			for (size_t i = 0; i < M; i++) mStorage[i][j] = c[i];
			return *this;
		}

		constexpr MVector<F, N> row(size_t i) const {
			return mStorage[i];
		}

		constexpr MMatrix& set_row(size_t i, const MVector<F, N>& r) {
			mStorage[i] = r;
			return *this;
		}

		// Just hammering out the API
		template<std::convertible_to<F> S>
		constexpr static MMatrix splat(S splat) {
			MMatrix<F, M, N> z;
			for (size_t i = 0; i < N; i++) {
				z.mStorage[i] = MVector<F,N>::splat(splat);
			}
			return z;
		}

		consteval static MMatrix zero() {
			return MMatrix();
		}

		consteval static MMatrix identity() requires (N==M) {
			auto mat = MMatrix::zero();
			for (size_t i = 0; i < N; i++) {
				mat.mStorage[i][i] = static_cast<F>(1);
			}
			return mat;
		}

		template<typename... Args>
			requires(sizeof...(Args) == M && std::conjunction_v<std::is_same<MVector<F,N>, Args>...>)
		constexpr static MMatrix from_rows(const Args&... rows) {
			auto mat = MMatrix::zero();
			size_t i = 0;
			(..., mat.set_row(i++, rows));
			return mat;
		}

		template<typename... Args>
			requires(sizeof...(Args) == N && std::conjunction_v<std::is_same<MVector<F, M>, Args>...>)
		constexpr static MMatrix from_cols(const Args&... cols) {
			auto mat = MMatrix::zero();
			size_t j = 0;
			(..., mat.set_col(j++, cols));
			return mat;
		}

		constexpr bool operator==(const MMatrix&) const = default;
		constexpr bool operator!=(const MMatrix&) const = default;

		#define MAT_DERIVE_BINOP(sym, in) \
		constexpr MMatrix operator sym (in other) const { \
			 auto copy = *this; copy sym##= other; return copy; \
		}

		constexpr MMatrix& operator+=(const MMatrix& other) {
			for (size_t i = 0; i < M; i++) mStorage[i] += other.mStorage[i];
			return *this;
		}
		MAT_DERIVE_BINOP(+, const MMatrix&);

		constexpr MMatrix& operator-=(const MMatrix& other) {
			for (size_t i = 0; i < M; i++) mStorage[i] -= other.mStorage[i];
			return *this;
		}
		MAT_DERIVE_BINOP(-, const MMatrix&);

		constexpr MVector<F, M> operator*(const MVector<F, N>& vec) const {
			auto result = MVector<F, M>::zero();
			for (size_t i = 0; i < M; ++i) {
				result[i] = dot(mStorage[i], vec);
			}
			return result;
		}

		template<std::convertible_to<F> S>
		constexpr MMatrix& operator*=(S scalar) {
			for (size_t i = 0; i < M; i++) mStorage[i] *= scalar;
			return *this;
		}
		template<std::convertible_to<F> S>
		MAT_DERIVE_BINOP(*, S);

		template<std::convertible_to<F> S>
		constexpr MMatrix& operator/=(S scalar) {
			F inv = static_cast<F>(1) / static_cast<F>(scalar);
			for (size_t i = 0; i < M; i++) mStorage[i] *= inv;
			return *this;
		}
		template<std::convertible_to<F> S>
		MAT_DERIVE_BINOP(/, S);

		#undef MAT_DERIVE_BINOP

		constexpr MMatrix<F, N, M> transpose() const {
			auto transposed = MMatrix<F,N,M>::zero();
			for (size_t j = 0; j < M; j++) {
				transposed.set_col(j, mStorage[j]);
			}
			return transposed;
		}

		template<size_t O>
		constexpr MMatrix<F, M, O> operator*(const MMatrix<F, N, O>& other) const {
			auto mul = MMatrix<F, M, O>::zero();
			for (size_t r = 0; r < M; r++) {
				auto in_row = this->row(r);
				auto out_row = MVector<F, O>::zero();
				for (size_t c = 0; c < O; c++) {
					out_row[c] = dot(in_row, other.col(c));
				}
				mul.set_row(r, out_row);
			}
			return mul;
		}

	private:
		template<bool Reduce>
		constexpr F gauss_eliminate() {
			F determinant = static_cast<F>(1);
			for (size_t i = 0, j = 0; i < M && j < N; ++i, ++j) {
				// Scan through remaining rows, find the one with the max value
				// in the j-th column.
				// Technically, we only need to find the first nonzero value...
				// but this gives better numerical stability
				F pivot = mStorage[i][j];
				size_t pivot_row = i;

				for (size_t k = i + 1; k < M; k++) {
					// Fun trick: fabs is slow because it needs to handle
					// a bunch of edge cases. But since we're comparing two
					// magnitudes, we can just compare the squares...
					F cur = mStorage[k][j];
					if (cur * cur > pivot * pivot) {
						pivot = cur;
						pivot_row = k;
					}
				}

				if (pivot == static_cast<F>(0)) {
					// every row had a zero in column j
					// on next iter, use row i, but with column j+1
					i -= 1;
					// Also, this means column j is entirely zero,
					// so the determinant of the MxM induced submatrix
					// is definitely zero!
					determinant = 0;
					continue;
				}

				// Get a version of the pivot row with the pivot
				// "normalized" so we don't have to divide each iteration
				MVector<F, N> prediv = mStorage[pivot_row] / pivot;
				// Ideally this wouldn't be necessary, but we sometimes
				// get values nearly imperceptibly off from 1 (but still
				// greater than the epislon).
				prediv[j] = 1;

				// Move the row with the max value into the uppermost
				// position in row-echelon form, making sure to update
				// the scale: swapping rows negates det, scaling a row
				// scales the det by that same amount.
				if (pivot_row != i) {
					// swap rows, update determinant scale accordingly
					mStorage[pivot_row] = mStorage[i];
					determinant = -determinant;
				}
				mStorage[i] = prediv;
				determinant *= pivot;

				// (If reducing) ensure the column is zero in all
				// rows above it by subtracting that row.
				if constexpr (Reduce) {
					for (size_t k = 0; k < i; k++) {
						mStorage[k] -= mStorage[k][j] * prediv;
					}
				}

				// Ensure the column is zero in all the rows
				// underneath it by subtracting that row.
				for (size_t k = i + 1; k < M; k++) {
					mStorage[k] -= mStorage[k][j] * prediv;
				}
			}
			return determinant;
		}

	public:
		// Converts the matrix into row-echelon form. Returns the determinant
		// of the matrix induced by the first M columns. The return value is
		// meaningless if M > N.
		constexpr F make_row_echelon() {
			return this->gauss_eliminate<false>();
		}
		constexpr MMatrix row_echelon() const {
			auto copy = *this;
			copy.make_row_echelon();
			return copy;
		}

		constexpr F make_reduced_row_echelon() {
			return this->gauss_eliminate<true>();
		}
		
		constexpr MMatrix reduced_row_echelon() const {
			auto copy = *this;
			copy.make_reduced_row_echelon();
			return copy;
		}

		template<typename _ = void> 
			requires(N == M)
		constexpr F determinant() const {
			if constexpr (N == 2) {
				return mStorage[0].x * mStorage[1].y - mStorage[0].y * mStorage[1].x;
			}
			auto copy = *this;
			return copy.make_row_echelon();
		}

		template<typename _ = void> 
			requires(N == M)
		constexpr MMatrix inverse() const {
			if constexpr (N == 2) {
				MMatrix<F, 2, 2> unscaled = {
					mStorage[1][1], -mStorage[0][1],
					-mStorage[1][0], mStorage[0][0]
				};
				return unscaled / this->determinant();
			}
			auto system = MMatrix<F, M, 2 * N>::zero();
			for (size_t i = 0; i < M; ++i) {
				auto sys_row = MVector<F, 2 * N>::zero();
				for (size_t j = 0; j < N; ++j) {
					sys_row[j] = mStorage[i][j];
				}
				sys_row[N + i] = static_cast<F>(1);
				system.set_row(i, sys_row);
			}
			system.make_reduced_row_echelon();
			auto inv = MMatrix<F, M, N>::zero();
			for (size_t i = 0; i < M; ++i) {
				MVector<F, N> inv_row = system.row(i).template slice<N, 2*N>();
				inv.set_row(i, inv_row);
			}
			return inv;
		}
		

		// We DON'T do rotate, scale, transform matrices here
		// rotate: only makes sense for square & different in each dim
		// scale: only makes sense for square matrices
		// transform: should be in MMatrixH subclass
	};

	template<std::floating_point F, size_t M, size_t N, std::convertible_to<F> S>
	inline constexpr MMatrix<F, M, N> operator*(S scalar, const MMatrix<F, M, N>& mat) {
		return mat * scalar;
	}

	template<std::floating_point F, size_t M, size_t N>
	inline std::ostream& operator<<(std::ostream& os, const MMatrix<F, M, N>& mat)
	{
		auto print_row = [&](const char* start, size_t j, const char* end) mutable {
			os << start;
			for (size_t i = 0; i < N; i++) os << " " << mat.col(i)[j];
			os << " " << end;
		};
		// I was using fancy Unicode box art here, but MSVC complains because 
		// Windows has terrible support for Unicode
		print_row("/", 0, "\\\n");

		DIAG_PUSHIGNORE_MSVC(6294); // This is *intentionally* never executed when M=2
		for (size_t j = 1; j < M - 1; j++) print_row("|", j, "|\n");
		DIAG_POP_MSVC();

		print_row("\\", M - 1, "/");
		return os;
	}

	template<std::floating_point F>
	inline bool aeq(F l, F r, F tolerance = std::numeric_limits<F>::epsilon()) {
		if (!std::isfinite(l)) {
			return !std::isfinite(r) && l == r;
		}
		else if (!std::isfinite(r)) {
			return false;
		}
		F mag = std::fmax(std::fabs(l), std::fabs(r));
		F scale = std::fmax(mag, static_cast<F>(1));
		return std::fabs(l - r) <= (tolerance * scale);
	}

	template<std::floating_point F, size_t C>
	inline bool aeq(const MVector<F, C>& left, const MVector<F, C>& right, F tolerance = std::numeric_limits<F>::epsilon()) {
		for (size_t i = 0; i < C; i++) {
			if (!aeq(left[i], right[i], tolerance)) return false;
		}
		return true;
	}

	template<std::floating_point F, size_t M, size_t N>
	inline bool aeq(const MMatrix<F, M, N>& left, const MMatrix<F, M, N>& right, F tolerance = std::numeric_limits<F>::epsilon()) {
		for (size_t i = 0; i < M; i++) {
			if (!aeq(left.row(i), right.row(i), tolerance)) return false;
		}
		return true;
	}

	template<std::floating_point F, size_t C, std::convertible_to<F> S>
	inline constexpr MVector<F, C> cubic_bezier(const MVector<F, C>& p0, const MVector<F, C>& cp0, const MVector<F, C>& cp1, const MVector<F, C>& p1, S factor) {
		F cvt = static_cast<F>(factor);
		MVector<F, 4> facs = { 1, cvt, cvt * cvt, cvt * cvt * cvt };
		constexpr static MMatrix<F, 4, 4> CUBIC_BEZIER_MATRIX = {
			1, 0, 0, 0,
			-3, 3, 0, 0,
			3, -6, 3, 0,
			-1, 3, -3, 1
		};
		auto result = MVector<F, C>::zero();
		for (size_t i = 0; i < C; ++i) {
			MVector<F, 4> gathered = { p0[i], cp0[i], cp1[i], p1[i] };
			result[i] = dot(facs, CUBIC_BEZIER_MATRIX * gathered);
		}
		return result;
	}
}

using vec2 = math::MVector<float, 2>;
using vec3 = math::MVector<float, 3>;
using vec4 = math::MVector<float, 4>;

using mat2 = math::MMatrix<float, 2, 2>;
using mat3 = math::MMatrix<float, 3, 3>;
using mat4 = math::MMatrix<float, 4, 4>;

using mat2x3 = math::MMatrix<float, 2, 3>;
using mat3x4 = math::MMatrix<float, 3, 4>;

using math::dot;
using math::cross;
using math::aeq;
using math::lerp;

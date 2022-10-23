#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <concepts>
#include <cmath>
#include <limits>
#include <iostream>

#include "util.h"

//! \file linalg.h
//! Graphics-oriented linear algebra.

template<typename T>
concept Numeric = std::is_floating_point_v<T> || std::is_integral_v<T>;

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
		constexpr T& operator[](size_t idx) { \
			check_member_layout(); \
			_IMPL_CONSTEVAL_ACCESS(idx, x, y, z, w, rest); \
			assert(idx < count); \
			char* member = reinterpret_cast<char*>(this) + sizeof(T) * idx; \
			return *reinterpret_cast<T*>(member); \
		} \
		constexpr const T& operator[](size_t idx) const { \
			check_member_layout(); \
			_IMPL_CONSTEVAL_ACCESS(idx, x, y, z, w, rest); \
			assert(idx < count); \
			const char* member = reinterpret_cast<const char*>(this) + sizeof(T) * idx; \
			return *reinterpret_cast<const T*>(member); \
		}

	template<Numeric T, size_t C>
	struct MVectorStorage {
		T x, y, z, w;
		T mExtra[C-4];
		IMPL_MEMBER_ACCESS(C, x, y, z, w, mExtra[idx - 4])

		constexpr MVectorStorage() : x(), y(), z(), w(), mExtra() {};

		template<typename ...Args>
			requires(sizeof...(Args) == C-4 && std::conjunction_v<std::is_same<T, Args>...>)
		constexpr MVectorStorage(T _x, T _y, T _z, T _w, Args... _extra)
			: x(_x), y(_y), z(_z), w(_w), mExtra{ _extra... } {}
	private:
		// These need to be in a function so they can refer to MVectorStorage.
		consteval static void check_member_layout() {
			static_assert(std::is_standard_layout<MVectorStorage>());
			static_assert(offsetof(MVectorStorage, x) == 0 * sizeof(T));
			static_assert(offsetof(MVectorStorage, y) == 1 * sizeof(T));
			static_assert(offsetof(MVectorStorage, z) == 2 * sizeof(T));
			static_assert(offsetof(MVectorStorage, w) == 3 * sizeof(T));
			static_assert(offsetof(MVectorStorage, mExtra) == 4 * sizeof(T));
		}
	};

	template<Numeric T>
	struct alignas(sizeof(T) * 4) MVectorStorage<T, 4> {
		T x, y, z, w;
		IMPL_MEMBER_ACCESS(4, x, y, z, w, w);
	private:
		// These need to be in a function so they can refer to MVectorStorage.
		consteval static void check_member_layout() {
			static_assert(std::is_standard_layout<MVectorStorage>());
			static_assert(offsetof(MVectorStorage, x) == 0 * sizeof(T));
			static_assert(offsetof(MVectorStorage, y) == 1 * sizeof(T));
			static_assert(offsetof(MVectorStorage, z) == 2 * sizeof(T));
			static_assert(offsetof(MVectorStorage, w) == 3 * sizeof(T));
		}
	};

	template<Numeric T>
	struct MVectorStorage<T, 3> {
		T x, y, z;
		IMPL_MEMBER_ACCESS(3, x, y, z, z, z);
	private:
		// These need to be in a function so they can refer to MVectorStorage.
		consteval static void check_member_layout() {
			static_assert(std::is_standard_layout<MVectorStorage>());
			static_assert(offsetof(MVectorStorage, x) == 0 * sizeof(T));
			static_assert(offsetof(MVectorStorage, y) == 1 * sizeof(T));
			static_assert(offsetof(MVectorStorage, z) == 2 * sizeof(T));
		}
	};
	template<Numeric T>
	struct MVectorStorage<T, 2> {
		T x, y;
		IMPL_MEMBER_ACCESS(2, x, y, y, y, y);
	private:
		// These need to be in a function so they can refer to MVectorStorage.
		consteval static void check_member_layout() {
			static_assert(std::is_standard_layout<MVectorStorage>());
			static_assert(offsetof(MVectorStorage, x) == 0 * sizeof(T));
			static_assert(offsetof(MVectorStorage, y) == 1 * sizeof(T));
		}
	};

	#undef IMPL_MEMBER_ACCESS
	#undef _IMPL_CONSTEVAL_ACCESS

	template<Numeric T, size_t C> 
		requires(2 <= C)
	struct MVector : public MVectorStorage<T, C> {
		/// The number of elements in vectors of this type.
		constexpr static size_t SIZE = C;

		/// Default constructor
		constexpr MVector() : MVectorStorage<T, C>{} {}

		/// Scalar "splat" scalar cast. Sets all elements to `splat`.
		template<std::convertible_to<T> S>
		explicit constexpr MVector(S splat) : MVector() {
			auto converted = static_cast<T>(splat);
			for (size_t i = 0; i < C; i++) {
				(*this)[i] = converted;
			}
		}

		/// Per-element cast conversion
		template<std::convertible_to<T> U>
		explicit constexpr MVector(const MVector<U, C>& other) : MVector() {
			for (size_t i = 0; i < C; i++) {
				(*this)[i] = static_cast<T>(other[i]);
			}
		}

		template<std::convertible_to<T> S>
		constexpr static MVector splat(S splat) {
			return static_cast<MVector>(splat);
		}

		consteval static MVector zero() {
			return MVector::splat(0);
		}

		

		/// Per-element bracket initialization syntax.
		template<typename ...Args>
			requires(sizeof...(Args) == C && std::conjunction_v<std::is_convertible<T, Args>...>)
		constexpr MVector(const Args&... args) : MVectorStorage<T,C> ( static_cast<T>(args)... ) {}

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
			return *this * static_cast<T>(-1);
		}

		/// Scalar multiplication.
		template<std::convertible_to<T> S>
		constexpr MVector& operator*=(S scalar) {
			auto scalar_cvt = static_cast<T>(scalar);
			for (size_t i = 0; i < C; i++) (*this)[i] *= scalar_cvt;
			return *this;
		}
		template<std::convertible_to<T> S>
		VEC_DERIVE_BINOP(*, S);

		/// Element-wise vector multiplication (not the dot product!)
		constexpr MVector& operator*=(const MVector& other) {
			for (size_t i = 0; i < C; i++) (*this)[i] *= other[i];
			return *this;
		}
		VEC_DERIVE_BINOP(*, const MVector&);

		/// Scalar division.
		template<std::convertible_to<T> S>
		constexpr MVector& operator/=(S scalar) {
			if constexpr (std::is_floating_point_v<T>) {
				// Find reciprocal and multiply by that,
				// since float division is super slow.
				auto inv = static_cast<T>(1) / static_cast<T>(scalar);
				return this->operator*=(inv);
			}
			else {
				auto converted = static_cast<T>(scalar);
				for (size_t i = 0; i < C; i++) (*this)[i] /= converted;
				return *this;
			}
			
		}
		template<std::convertible_to<T> S>
		VEC_DERIVE_BINOP(/, S)

		/// Element-wise vector division.
		constexpr MVector& operator/=(const MVector& other) {
			for (size_t i = 0; i < C; i++) (*this)[i] /= other[i];
			return *this;
		}
		VEC_DERIVE_BINOP(/, const MVector&)

		#undef VEC_DERIVE_BINOP

		// Returns the magnitude of this vector.
		constexpr T mag() const {
			return std::sqrt(dot(*this, *this));
		}

		template<typename _ = void>
			requires(std::is_floating_point_v<T>)
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
		constexpr MVector<T, End - Start> slice() const {
			auto sliced = MVector<T, End - Start>::zero();
			for (size_t i = 0; i < End - Start; i++) {
				sliced[i] = (*this)[i+Start];
			}
			return sliced;
		}

		template<size_t D, std::convertible_to<T> S>
			requires(D > C)
		constexpr MVector<T, D> extend(S fill) const {
			auto converted = static_cast<T>(fill);
			MVector<T, D> extended;
			for (size_t i = 0; i < C; ++i) extended[i] = (*this)[i];
			for (size_t i = C; i < D; ++i) extended[i] = fill;
			return extended;
		}

		// Homogenizes the current vector
		constexpr MVector<T, C + 1> hmg() const {
			return extend<C + 1>(static_cast<T>(1));
		}
	};

	template<Numeric T, size_t C>
	inline std::ostream& operator<<(std::ostream& os, const MVector<T,C>& vec)
	{
		os << "(" << vec[0]; 
		for (size_t i = 1; i < C; i++) os << ", " << vec[i];
		os << ")";
		return os;
	}

	template<Numeric T, size_t C, std::convertible_to<T> S>
	inline constexpr MVector<T,C> operator*(S scalar, const MVector<T, C>& vec) {
		return vec * scalar;
	}


	template<Numeric T, size_t C>
	inline constexpr T dot(const MVector<T, C>& l, const MVector<T, C>& r) {
		auto sum = static_cast<T>(0);
		for (size_t i = 0; i < C; i++) sum += l[i] * r[i];
		return sum;
	}

	template<std::floating_point T, size_t C, std::convertible_to<T> S>
	inline constexpr MVector<T,C> lerp(const MVector<T, C>& p0, const MVector<T, C>& p1, S factor) {
		auto factor_cvt = static_cast<T>(factor);
		return (static_cast<T>(1) - factor_cvt) * p0 + (factor_cvt) * p1;
	}

	template<Numeric T>
	inline constexpr MVector<T, 3> cross(const MVector<T, 3>& a, const MVector<T, 3>& b) {
		return MVector<T, 3> {
			a.y* b.z - a.z * b.y,
			a.z* b.x - a.x * b.z,
			a.x* b.y - a.y * b.x
		};
	}

	template<Numeric T, size_t C>
	inline MVector<T, C> vmax(const MVector<T, C>& left, const MVector<T, C>& right) {
		auto out = MVector<T, C>::zero();
		for (size_t i = 0; i < C; i++) out[i] = std::fmax(left[i], right[i]);
		return out;
	}

	template<Numeric T, size_t C>
	inline MVector<T, C> vmin(const MVector<T, C>& left, const MVector<T, C>& right) {
		auto out = MVector<T, C>::zero();
		for (size_t i = 0; i < C; i++) out[i] = std::fmin(left[i], right[i]);
		return out;
	}

	template<Numeric T, size_t M, size_t N>
		requires (2 <= M && 2 <= N)
	class MMatrix {
		// Row-major storage.
		std::array<MVector<T, N>, M> mStorage;

	public:
		/// Default constructor (all zeroes).
		constexpr MMatrix() : mStorage() {}

		/// Entry-by-entry initialization.
		template<typename... Args>
			requires(sizeof...(Args) == N * M && std::conjunction_v<std::is_convertible<T, Args>...>)
		constexpr MMatrix(const Args&... args) : MMatrix() {
			// This initialization code is absolutely batshit insane
			auto write_entry = [](MMatrix& mat, size_t& k, T val) {
				mat.mStorage[k / N][k % N] = val;
				++k;
			};
			size_t i = 0;
			(..., write_entry(*this, i, static_cast<T>(args)));
		}

		constexpr MVector<T, M> col(size_t j) const {
			auto rv = MVector<T, M>::zero();
			for (size_t i = 0; i < M; i++) rv[i] = mStorage[i][j];
			return rv;
		}

		constexpr MMatrix& set_col(size_t j, const MVector<T, M>& c) {
			for (size_t i = 0; i < M; i++) mStorage[i][j] = c[i];
			return *this;
		}

		constexpr MVector<T, N> row(size_t i) const {
			return mStorage[i];
		}

		constexpr MMatrix& set_row(size_t i, const MVector<T, N>& r) {
			mStorage[i] = r;
			return *this;
		}

		// Just hammering out the API
		template<std::convertible_to<T> S>
		constexpr static MMatrix splat(S splat) {
			MMatrix<T, M, N> z;
			for (size_t i = 0; i < N; i++) {
				z.mStorage[i] = MVector<T,N>::splat(splat);
			}
			return z;
		}

		consteval static MMatrix zero() {
			return MMatrix();
		}

		consteval static MMatrix ident() requires (N==M) {
			return MMatrix::scale(1);
		}

		// Uniform scale
		template<std::convertible_to<T> S>
			requires(N == M)
		consteval static MMatrix scale(S scalar){
			auto converted = static_cast<T>(scalar);
			auto mat = MMatrix::zero();
			for (size_t i = 0; i < N; i++) {
				mat.mStorage[i][i] = converted;
			}
			return mat;
		}

		template<typename... Args>
			requires(N == M && sizeof...(Args) == M && std::conjunction_v<std::is_convertible<T, Args>...>)
		consteval static MMatrix diag(const Args&... args) {
			auto mat = MMatrix::zero();
			size_t i = 0;
			// C++ makes me so sad all the time
			(..., (mat.mStorage[i][i] = static_cast<T>(args), ++i));
			return mat;
		}

		template<typename... Args>
			requires(sizeof...(Args) == M && std::conjunction_v<std::is_same<MVector<T,N>, Args>...>)
		constexpr static MMatrix from_rows(const Args&... rows) {
			auto mat = MMatrix::zero();
			size_t i = 0;
			(..., mat.set_row(i++, rows));
			return mat;
		}

		template<typename... Args>
			requires(sizeof...(Args) == N && std::conjunction_v<std::is_same<MVector<T, M>, Args>...>)
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

		constexpr MVector<T, M> operator*(const MVector<T, N>& vec) const {
			auto result = MVector<T, M>::zero();
			for (size_t i = 0; i < M; ++i) {
				result[i] = dot(mStorage[i], vec);
			}
			return result;
		}

		template<std::convertible_to<T> S>
		constexpr MMatrix& operator*=(S scalar) {
			for (size_t i = 0; i < M; i++) mStorage[i] *= scalar;
			return *this;
		}
		template<std::convertible_to<T> S>
		MAT_DERIVE_BINOP(*, S);

		template<std::convertible_to<T> S>
		constexpr MMatrix& operator/=(S scalar) {
			if constexpr (std::is_floating_point_v<T>) {
				auto inv = static_cast<T>(1) / static_cast<T>(scalar);
				for (size_t i = 0; i < M; i++) mStorage[i] *= inv;
			}
			else {
				auto converted = static_cast<T>(scalar);
				for (size_t i = 0; i < M; i++) mStorage[i] /= converted;
			}
			return *this;
		}
		template<std::convertible_to<T> S>
		MAT_DERIVE_BINOP(/, S);

		#undef MAT_DERIVE_BINOP

		constexpr MMatrix<T, N, M> transpose() const {
			auto transposed = MMatrix<T,N,M>::zero();
			for (size_t j = 0; j < M; j++) {
				transposed.set_col(j, mStorage[j]);
			}
			return transposed;
		}

		template<size_t O>
		constexpr MMatrix<T, M, O> operator*(const MMatrix<T, N, O>& other) const {
			auto mul = MMatrix<T, M, O>::zero();
			for (size_t r = 0; r < M; r++) {
				auto in_row = this->row(r);
				auto out_row = MVector<T, O>::zero();
				for (size_t c = 0; c < O; c++) {
					out_row[c] = dot(in_row, other.col(c));
				}
				mul.set_row(r, out_row);
			}
			return mul;
		}

	private:
		template<bool Reduce>
		constexpr T gauss_eliminate() requires(std::is_floating_point_v<T>) {
			T determinant = static_cast<T>(1);
			for (size_t i = 0, j = 0; i < M && j < N; ++i, ++j) {
				// Scan through remaining rows, find the one with the max value
				// in the j-th column.
				// Technically, we only need to find the first nonzero value...
				// but this gives better numerical stability
				T pivot = mStorage[i][j];
				size_t pivot_row = i;

				for (size_t k = i + 1; k < M; k++) {
					// Fun trick: fabs is slow because it needs to handle
					// a bunch of edge cases. But since we're comparing two
					// magnitudes, we can just compare the squares...
					T cur = mStorage[k][j];
					if (cur * cur > pivot * pivot) {
						pivot = cur;
						pivot_row = k;
					}
				}

				if (pivot == static_cast<T>(0)) {
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
				MVector<T, N> prediv = mStorage[pivot_row] / pivot;
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
		constexpr T make_row_echelon() {
			return this->gauss_eliminate<false>();
		}
		constexpr MMatrix row_echelon() const {
			auto copy = *this;
			copy.make_row_echelon();
			return copy;
		}

		constexpr T make_reduced_row_echelon() {
			return this->gauss_eliminate<true>();
		}
		
		constexpr MMatrix reduced_row_echelon() const {
			auto copy = *this;
			copy.make_reduced_row_echelon();
			return copy;
		}

		constexpr T determinant() const requires(N == M) {
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
				MMatrix<T, 2, 2> unscaled = {
					mStorage[1][1], -mStorage[0][1],
					-mStorage[1][0], mStorage[0][0]
				};
				return unscaled / this->determinant();
			}
			auto system = MMatrix<T, M, 2 * N>::zero();
			for (size_t i = 0; i < M; ++i) {
				auto sys_row = MVector<T, 2 * N>::zero();
				for (size_t j = 0; j < N; ++j) {
					sys_row[j] = mStorage[i][j];
				}
				sys_row[N + i] = static_cast<T>(1);
				system.set_row(i, sys_row);
			}
			system.make_reduced_row_echelon();
			auto inv = MMatrix<T, M, N>::zero();
			for (size_t i = 0; i < M; ++i) {
				MVector<T, N> inv_row = system.row(i).template slice<N, 2*N>();
				inv.set_row(i, inv_row);
			}
			return inv;
		}
	};

	template<Numeric T, size_t M, size_t N, std::convertible_to<T> S>
	inline constexpr MMatrix<T, M, N> operator*(S scalar, const MMatrix<T, M, N>& mat) {
		return mat * scalar;
	}

	template<Numeric T, size_t M, size_t N>
	inline std::ostream& operator<<(std::ostream& os, const MMatrix<T, M, N>& mat)
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

	template<std::floating_point T>
	inline bool aeq(T l, T r, T tolerance = std::numeric_limits<T>::epsilon()) {
		if (!std::isfinite(l)) {
			return !std::isfinite(r) && l == r;
		}
		else if (!std::isfinite(r)) {
			return false;
		}
		T mag = std::fmax(std::fabs(l), std::fabs(r));
		T scale = std::fmax(mag, static_cast<T>(1));
		return std::fabs(l - r) <= (tolerance * scale);
	}

	template<std::floating_point T, size_t C>
	inline bool aeq(const MVector<T, C>& left, const MVector<T, C>& right, T tolerance = std::numeric_limits<T>::epsilon()) {
		for (size_t i = 0; i < C; i++) {
			if (!aeq(left[i], right[i], tolerance)) return false;
		}
		return true;
	}

	template<std::floating_point T, size_t M, size_t N>
	inline bool aeq(const MMatrix<T, M, N>& left, const MMatrix<T, M, N>& right, T tolerance = std::numeric_limits<T>::epsilon()) {
		for (size_t i = 0; i < M; i++) {
			if (!aeq(left.row(i), right.row(i), tolerance)) return false;
		}
		return true;
	}

	namespace {
		template<Numeric T>
		static constexpr MMatrix<T, 4, 4> CUBIC_BEZIER_MATRIX = {
			1, 0, 0, 0,
			-3, 3, 0, 0,
			3, -6, 3, 0,
			-1, 3, -3, 1
		};
	}
	template<std::floating_point T, size_t C, std::convertible_to<T> S>
	inline constexpr MVector<T, C> cubic_bezier(const MVector<T, C>& p0, const MVector<T, C>& cp0, const MVector<T, C>& cp1, const MVector<T, C>& p1, S factor) {
		T cvt = static_cast<T>(factor);
		MVector<T, 4> facs = { 1, cvt, cvt * cvt, cvt * cvt * cvt };
		auto result = MVector<T, C>::zero();
		for (size_t i = 0; i < C; ++i) {
			MVector<T, 4> gathered = { p0[i], cp0[i], cp1[i], p1[i] };
			result[i] = dot(facs, CUBIC_BEZIER_MATRIX<T> * gathered);
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

using ivec2 = math::MVector<int, 2>;
using ivec3 = math::MVector<int, 3>;
using ivec4 = math::MVector<int, 4>;

using math::dot;
using math::cross;
using math::aeq;
using math::lerp;

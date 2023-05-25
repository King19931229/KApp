#pragma once
#include "KBase/Interface/IKAssetLoader.h"
#include "KBase/Publish/KAABBBox.h"
#include <unordered_set>
#include <unordered_map>
#include <tuple>
#include <queue>

// LUP factorization using Doolittle's method with partial pivoting
template<typename T>
bool LUPFactorize(T* A, uint32_t* pivot, uint32_t size, T epsilon)
{
	for (uint32_t i = 0; i < size; i++)
	{
		pivot[i] = i;
	}

	for (uint32_t i = 0; i < size; i++)
	{
		// Find largest pivot in column
		T		maxValue = abs(A[size * i + i]);
		int32_t	maxIndex = i;

		for (uint32_t j = i + 1; j < size; j++)
		{
			T absValue = abs(A[size * j + i]);
			if (absValue > maxValue)
			{
				maxValue = absValue;
				maxIndex = j;
			}
		}

		if (maxValue < epsilon)
		{
			// Matrix is singular
			return false;
		}

		// Swap rows pivoting MaxValue to the diagonal
		if (maxIndex != i)
		{
			std::swap(pivot[i], pivot[maxIndex]);

			for (uint32_t j = 0; j < size; j++)
				std::swap(A[size * i + j], A[size * maxIndex + j]);
		}

		// Gaussian elimination
		for (uint32_t j = i + 1; j < size; j++)
		{
			A[size * j + i] /= A[size * i + i];

			for (uint32_t k = i + 1; k < size; k++)
				A[size * j + k] -= A[size * j + i] * A[size * i + k];
		}
	}

	return true;
}

// Solve system of equations A*x = b
template< typename T >
void LUPSolve(const T* LU, const uint32_t* pivot, uint32_t size, const T* b, T* x)
{
	for (uint32_t i = 0; i < size; i++)
	{
		x[i] = b[pivot[i]];

		for (uint32_t j = 0; j < i; j++)
			x[i] -= LU[size * i + j] * x[j];
	}

	for (int32_t i = (int32_t)size - 1; i >= 0; i--)
	{
		for (uint32_t j = i + 1; j < size; j++)
			x[i] -= LU[size * i + j] * x[j];

		// Diagonal was filled with max values, all greater than Epsilon
		x[i] /= LU[size * i + i];
	}
}

// Newton's method iterative refinement.
template< typename T >
bool LUPSolveIterate(const T* A, const T* LU, const uint32_t* pivot, uint32_t size, const T* b, T* x)
{
	T* residual = (T*)malloc(2 * size * sizeof(T));
	T* error = residual + size;

	LUPSolve(LU, pivot, size, b, x);

	bool bCloseEnough = false;
	for (uint32_t k = 0; k < 4; k++)
	{
		for (uint32_t i = 0; i < size; i++)
		{
			residual[i] = b[i];

			for (uint32_t j = 0; j < size; j++)
			{
				residual[i] -= A[size * i + j] * x[j];
			}
		}

		LUPSolve(LU, pivot, size, residual, error);

		T meanSquaredError = 0.0;
		for (uint32_t i = 0; i < size; i++)
		{
			x[i] += error[i];
			meanSquaredError += error[i] * error[i];
		}

		if (meanSquaredError < 1e-4f)
		{
			bCloseEnough = true;
			break;
		}
	}

	free(residual);
	return bCloseEnough;
}

template<typename T, uint32_t Dimension>
struct KVector
{
	static_assert(Dimension >= 1, "Dimension must >= 1");
	T v[Dimension];

	KVector()
	{
		for (uint32_t i = 0; i < Dimension; ++i)
			v[i] = 0;
	}

	T SquareLength() const
	{
		T res = 0;
		for (uint32_t i = 0; i < Dimension; ++i)
			res += v[i] * v[i];
		return res;
	}

	T Length() const
	{
		return (T)sqrt(SquareLength());
	}

	T Dot(const KVector& rhs)
	{
		T res = 0;
		for (uint32_t i = 0; i < Dimension; ++i)
			res += v[i] * rhs.v[i];
		return res;
	}

	KVector operator-() const
	{
		KVector res;
		for (uint32_t i = 0; i < Dimension; ++i)
			res.v[i] = -v[i];
		return res;
	}

	KVector operator*(T factor) const
	{
		KVector res;
		for (uint32_t i = 0; i < Dimension; ++i)
			res.v[i] = v[i] * factor;
		return res;
	}

	KVector operator/(T factor) const
	{
		KVector res;
		for (uint32_t i = 0; i < Dimension; ++i)
			res.v[i] = v[i] / factor;
		return res;
	}

	KVector& operator*=(T factor)
	{
		for (uint32_t i = 0; i < Dimension; ++i)
			v[i] *= factor;
		return *this;
	}

	KVector& operator/=(T factor)
	{
		for (uint32_t i = 0; i < Dimension; ++i)
			v[i] /= factor;
		return *this;
	}

	KVector operator+(const KVector& rhs) const
	{
		KVector res;
		for (uint32_t i = 0; i < Dimension; ++i)
			res.v[i] = v[i] + rhs.v[i];
		return res;
	}

	KVector operator-(const KVector& rhs) const
	{
		KVector res;
		for (uint32_t i = 0; i < Dimension; ++i)
			res.v[i] = v[i] - rhs.v[i];
		return res;
	}

	KVector& operator+=(const KVector& rhs)
	{
		for (uint32_t i = 0; i < Dimension; ++i)
			v[i] += rhs.v[i];
		return *this;
	}

	KVector& operator-=(const KVector& rhs)
	{
		for (uint32_t i = 0; i < Dimension; ++i)
			v[i] -= rhs.v[i];
		return *this;
	}
};

template<typename T, uint32_t Row, uint32_t Column>
struct KMatrix
{
	static_assert(Row >= 1, "Row must >= 1");
	static_assert(Column >= 1, "Column must >= 1");
	T m[Row][Column];

	KMatrix()
	{
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				m[i][j] = 0;
			}
		}
	}

	KMatrix(T val)
	{
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				if (i == j)
				{
					m[i][j] = val;
				}
				else
				{
					m[i][j] = 0;
				}
			}
		}
	}

	KMatrix<T, Column, Row> Transpose() const
	{
		KMatrix<T, Column, Row> res;
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				res.m[j][i] = m[i][j];
			}
		}
		return res;
	}

	KMatrix operator-() const
	{
		KMatrix res;
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				res.m[i][j] = -m[i][j];
			}
		}
		return res;
	}

	KMatrix operator*(T factor) const
	{
		KMatrix res;
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				res.m[i][j] = m[i][j] * factor;
			}
		}
		return res;
	}

	KMatrix operator/(T factor) const
	{
		KMatrix res;
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				res.m[i][j] = m[i][j] / factor;
			}
		}
		return res;
	}

	KMatrix& operator*=(T factor)
	{
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				m[i][j] *= factor;
			}
		}
		return *this;
	}

	KMatrix& operator/=(T factor)
	{
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				m[i][j] /= factor;
			}
		}
		return *this;
	}

	KMatrix operator+(const KMatrix& rhs) const
	{
		KMatrix res;
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				res.m[i][j] = m[i][j] + rhs.m[i][j];
			}
		}
		return res;
	}

	KMatrix operator-(const KMatrix& rhs) const
	{
		KMatrix res;
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				res.m[i][j] = m[i][j] - rhs.m[i][j];
			}
		}
		return res;
	}

	KMatrix& operator+=(const KMatrix& rhs)
	{
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				m[i][j] += rhs.m[i][j];
			}
		}
		return *this;
	}

	KMatrix& operator-=(const KMatrix& rhs)
	{
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				m[i][j] -= rhs.m[i][j];
			}
		}
		return *this;
	}
};

template<typename T, uint32_t Row, uint32_t Column>
KVector<T, Column> Mul(const KVector<T, Row>& lhs, const KMatrix<T, Row, Column>& rhs)
{
	KVector<T, Column> res;
	for (uint32_t j = 0; j < Column; ++j)
	{
		res.v[j] = 0;
		for (uint32_t i = 0; i < Row; ++i)
		{
			res.v[j] += lhs.v[i] * rhs.m[i][j];
		}
	}
	return res;
}

template<typename T, uint32_t Row, uint32_t Column>
KVector<T, Row> Mul(const KMatrix<T, Row, Column>& lhs, const KVector<T, Column>& rhs)
{
	KVector<T, Row> res;
	for (uint32_t i = 0; i < Row; ++i)
	{
		res.v[i] = 0;
		for (uint32_t j = 0; j < Column; ++j)
		{
			res.v[i] += lhs.m[i][j] * rhs.v[j];
		}
	}
	return res;
}

template<typename T, uint32_t Row, uint32_t Column>
KMatrix<T, Row, Column> Mul(const KVector<T, Column>& lhs, const KVector<T, Row>& rhs)
{
	KMatrix<T, Row, Column> res;
	for (uint32_t i = 0; i < Row; ++i)
	{
		for (uint32_t j = 0; j < Column; ++j)
		{
			res.m[i][j] = lhs.v[j] * rhs.v[i];
		}
	}
	return res;
}

template<typename T, uint32_t Row, uint32_t Conn, uint32_t Column>
KMatrix<T, Row, Column> Mul(const KMatrix<T, Row, Conn>& lhs, const KMatrix<T, Conn, Column>& rhs)
{
	KMatrix<T, Row, Column> res;
	for (uint32_t i = 0; i < Row; ++i)
	{
		for (uint32_t j = 0; j < Column; ++j)
		{
			res.m[i][j] = 0;
			for (uint32_t k = 0; k < Conn; ++k)
			{
				res.m[i][j] += lhs.m[i][k] * rhs.m[k][j];
			}
		}
	}
	return res;
}

template<typename T, uint32_t Dimension>
struct KQuadric
{
	static_assert(Dimension >= 1, "Dimension must >= 1");
	constexpr static uint32_t Size = (Dimension + 1) * Dimension / 2;

	T a[Size];
	T b[Dimension];
	T c;

	KQuadric()
	{
		for (uint32_t i = 0; i < Size; ++i)
			a[i] = 0;
		for (uint32_t i = 0; i < Dimension; ++i)
			b[i] = 0;
		c = 0;
	}

	KQuadric operator*(T factor) const
	{
		KQuadric res;
		for (uint32_t i = 0; i < Size; ++i)
			res.a[i] = a[i] * factor;
		for (uint32_t i = 0; i < Dimension; ++i)
			res.b[i] = b[i] * factor;
		res.c = c * factor;
		return res;
	}

	KQuadric operator/(T factor) const
	{
		KQuadric res;
		for (uint32_t i = 0; i < Size; ++i)
			res.a[i] = a[i] / factor;
		for (uint32_t i = 0; i < Dimension; ++i)
			res.b[i] = b[i] / factor;
		res.c = c / factor;
		return res;
	}

	KQuadric& operator*=(T factor)
	{
		for (uint32_t i = 0; i < Size; ++i)
			a[i] *= factor;
		for (uint32_t i = 0; i < Dimension; ++i)
			b[i] *= factor;
		c *= factor;
		return *this;
	}

	KQuadric& operator/=(T factor)
	{
		for (uint32_t i = 0; i < Size; ++i)
			a[i] /= factor;
		for (uint32_t i = 0; i < Dimension; ++i)
			b[i] /= factor;
		c /= factor;
		return *this;
	}

	KQuadric operator+(const KQuadric& rhs) const
	{
		KQuadric res;
		for (uint32_t i = 0; i < Size; ++i)
			res.a[i] = a[i] + rhs.a[i];
		for (uint32_t i = 0; i < Dimension; ++i)
			res.b[i] = b[i] + rhs.b[i];
		res.c = c + rhs.c;
		return res;
	}

	KQuadric operator-(const KQuadric& rhs) const
	{
		KQuadric res;
		for (uint32_t i = 0; i < Size; ++i)
			res.a[i] = a[i] - rhs.a[i];
		for (uint32_t i = 0; i < Dimension; ++i)
			res.b[i] = b[i] - rhs.b[i];
		res.c = c - rhs.c;
		return res;
	}

	KQuadric& operator+=(const KQuadric& rhs)
	{
		for (uint32_t i = 0; i < Size; ++i)
			a[i] += rhs.a[i];
		for (uint32_t i = 0; i < Dimension; ++i)
			b[i] += rhs.b[i];
		c += rhs.c;
		return *this;
	}

	KQuadric& operator-=(const KQuadric& rhs)
	{
		for (uint32_t i = 0; i < Size; ++i)
			a[i] -= rhs.a[i];
		for (uint32_t i = 0; i < Dimension; ++i)
			b[i] -= rhs.b[i];
		c -= rhs.c;
		return *this;
	}

	uint32_t PosToAIndex(uint32_t i, uint32_t j) const
	{
		if (j < i)
		{
			std::swap(i, j);
		}
		if (i < Dimension && j < Dimension)
		{
			return i * Dimension + j - (i * i + i) / 2;
		}
		else
		{
			assert(false);
			return Size;
		}
	}

	bool SetA(int32_t i, int32_t j, T value)
	{
		uint32_t index = PosToAIndex(i, j);
		if (index != Size)
		{
			a[index] = value;
			return true;
		}
		return false;
	}

	T& GetA(int32_t i, int32_t j)
	{
		return a[PosToAIndex(i, j)];
	}

	T Error(T* v) const
	{
		T error = 0;

		// vT * a * v
		for (uint32_t i = 0; i < Dimension; ++i)
		{
			for (uint32_t k = 0; k < Dimension; ++k)
			{
				error += v[k] * a[PosToAIndex(k, i)] * v[i];
			}
		}

		// 2 * bT * v
		for (uint32_t i = 0; i < Dimension; ++i)
		{
			error += 2 * b[i] * v[i];
		}

		// c
		error += c;

		return error;
	}

	bool Optimal(T* x) const
	{
		// Solve a * x = -b
		uint32_t pivot[Dimension] = { 0 };

		KMatrix<T, Dimension, Dimension> A;

		for (uint32_t i = 0; i < Dimension; ++i)
		{
			for (uint32_t j = 0; j < Dimension; ++j)
			{
				A.m[i][j] = -a[PosToAIndex(i, j)];
			}
		}

		if (LUPFactorize(&A.m[0][0], pivot, Dimension, 1e-3f))
		{
			LUPSolve(&A.m[0][0], pivot, Dimension, b, x);
			return true;
		}
		else
		{
			return false;
		}
	}

	bool OptimalVolume(T* x) const
	{
		return Optimal(x);
	}
};

template<typename T, uint32_t Attr>
struct KAttrQuadric
{
	constexpr static uint32_t Size = Attr + 3;
	typedef KVector<T, 3> Vector;
	typedef KMatrix<T, 3, 3> Matrix;
	Matrix	nxn_gxg;
	T		d2_dm2;
	T		diagonal;
	T		dm[Attr];
	Vector	gm[Attr];
	Vector	dn_dg;
	Vector	n;
	T		d;

	KAttrQuadric()
	{
		for (uint32_t i = 0; i < Attr; ++i)
		{
			dm[i] = 0;
		}
		d2_dm2 = 0;
		diagonal = 0;
		d = 0;
	}

	KAttrQuadric(const Vector& p0, const Vector& p1, const Vector& p2, T* in_m)
	{
		glm::tvec3<T> v0 = glm::tvec3<T>(p0.v[0], p0.v[1], p0.v[2]);
		glm::tvec3<T> v1 = glm::tvec3<T>(p1.v[0], p1.v[1], p1.v[2]);
		glm::tvec3<T> v2 = glm::tvec3<T>(p2.v[0], p2.v[1], p2.v[2]);

		glm::tvec3<T> v01 = v1 - v0;
		glm::tvec3<T> v02 = v2 - v0;

		glm::tvec3<T> normal = glm::normalize(glm::cross(v01, v02));

		n.v[0] = normal[0];
		n.v[1] = normal[1];
		n.v[2] = normal[2];

		d = -glm::dot(normal, v0);

		dn_dg = n * d;
		d2_dm2 = d * d;

		for (uint32_t i = 0; i < 3; ++i)
		{
			for (uint32_t j = 0; j < 3; ++j)
			{
				nxn_gxg.m[i][j] = n.v[i] * n.v[j];
			}
		}

#define COMPUTE_ATTR 1
#define USE_4X4 1

#if COMPUTE_ATTR
#if USE_4X4
		KMatrix<T, 4, 4> A;

		for (uint32_t j = 0; j < 3; ++j)
		{
			A.m[0][j] = p0.v[j];
			A.m[1][j] = p1.v[j];
			A.m[2][j] = p2.v[j];
			A.m[3][j] = n.v[j];
		}

		A.m[0][3] = A.m[1][3] = A.m[2][3] = 1;
		A.m[3][3] = 0;

		uint32_t pivot[4];
		bool bInvertable = LUPFactorize(&A.m[0][0], pivot, 4, (T)1e-12f);
#else
		KMatrix<T, 3, 3> A;
		
		A.m[0][0] = v01[0];		A.m[0][1] = v01[1];		A.m[0][2] = v01[2];
		A.m[1][0] = v02[0];		A.m[1][1] = v02[1];		A.m[1][2] = v02[2];
		A.m[2][0] = normal[0];	A.m[2][1] = normal[1];	A.m[2][2] = normal[2];

		uint32_t pivot[3];
		bool bInvertable = LUPFactorize(&A.m[0][0], pivot, 3, 1e-12f);
#endif
		for (uint32_t k = 0; k < Attr; ++k)
		{
			if (bInvertable)
			{
				T a[3] = { in_m[k], in_m[Attr + k], in_m[2 * Attr + k] };
#if USE_4X4
				T b[4] = { a[0], a[1], a[2], 0 };
				T x[4] = { 0, 0, 0, 0 };
				LUPSolve(&A.m[0][0], pivot, 4, b, x);
				gm[k].v[0]	= x[0];
				gm[k].v[1]	= x[1];
				gm[k].v[2]	= x[2];
				dm[k]		= x[3];
#else
				T b[3] = { a[2] - a[0], a[1] - a[0], 0 };
				T x[3] = { 0, 0, 0 };
				LUPSolve(&A.m[0][0], pivot, 3, b, x);
				gm[k].v[0]	= x[0];
				gm[k].v[1]	= x[1];
				gm[k].v[2]	= x[2];
				dm[k]		= a[0] - x[0] * p0.v[0] - x[1] * p0.v[1] - x[2] * p0.v[2];
#endif
				T ca[3] = { 0, 0, 0 };

				ca[0] = gm[k].Dot(p0) + dm[k];
				ca[1] = gm[k].Dot(p1) + dm[k];
				ca[2] = gm[k].Dot(p2) + dm[k];

				T diff[3] = { 0, 0, 0 };
				T diffSum = n.Dot(gm[k]);
				for (uint32_t i = 0; i < 3; ++i)
				{
					diff[i] = abs(a[i] - ca[i]);
					diffSum += diff[i];
				}
				if (diffSum > 1e-3f)
				{
					int d = 0;
				}
			}
			else
			{
				gm[k].v[0]	= 0;
				gm[k].v[1]	= 0;
				gm[k].v[2]	= 0;
				dm[k] = (in_m[k] + in_m[Attr + k] + in_m[2 * Attr + k]) / 3.0f;
			}

			dn_dg += gm[k] * dm[k];
			d2_dm2 += dm[k] * dm[k];

			for (uint32_t i = 0; i < 3; ++i)
			{
				for (uint32_t j = 0; j < 3; ++j)
				{
					nxn_gxg.m[i][j] += gm[k].v[i] * gm[k].v[j];
				}
			}
		}

		diagonal = 1;
#else
		for (uint32_t i = 0; i < Attr; ++i)
		{
			gm[i].v[0] = 0;
			gm[i].v[1] = 0;
			gm[i].v[2] = 0;
			dm[i] = 0;
		}
		diagonal = 1;
		d = 0;
#endif
	}

	KAttrQuadric operator*(T factor) const
	{
		KAttrQuadric res;
		res.nxn_gxg = nxn_gxg * factor;
		res.d2_dm2 = d2_dm2 * factor;
		res.dn_dg = dn_dg * factor;
		res.diagonal = diagonal * factor;
		for (uint32_t i = 0; i < Attr; ++i)
		{
			res.dm[i] = dm[i] * factor;
			res.gm[i] = gm[i] * factor;
		}
		res.n = n * factor;
		res.d = d * factor;
		return res;
	}

	KAttrQuadric operator/(T factor) const
	{
		KAttrQuadric res;
		res.nxn_gxg = nxn_gxg / factor;
		res.d2_dm2 = d2_dm2 / factor;
		res.dn_dg = dn_dg / factor;
		res.diagonal = diagonal / factor;
		for (uint32_t i = 0; i < Attr; ++i)
		{
			res.dm[i] = dm[i] / factor;
			res.gm[i] = gm[i] / factor;
		}
		res.n = n / factor;
		res.d = d / factor;
		return res;
	}

	KAttrQuadric& operator*=(T factor)
	{
		nxn_gxg *= factor;
		d2_dm2 *= factor;
		dn_dg *= factor;
		diagonal *= factor;
		for (uint32_t i = 0; i < Attr; ++i)
		{
			dm[i] *= factor;
			gm[i] *= factor;
		}
		n *= factor;
		d *= factor;
		return *this;
	}

	KAttrQuadric& operator/=(T factor)
	{
		nxn_gxg /= factor;
		d2_dm2 /= factor;
		dn_dg /= factor;
		diagonal /= factor;
		for (uint32_t i = 0; i < Attr; ++i)
		{
			dm[i] /= factor;
			gm[i] /= factor;
		}
		n /= factor;
		d /= factor;
		return *this;
	}

	KAttrQuadric operator+(const KAttrQuadric& rhs) const
	{
		KAttrQuadric res;
		res.nxn_gxg = nxn_gxg + rhs.nxn_gxg;
		res.d2_dm2 = d2_dm2 + rhs.d2_dm2;
		res.dn_dg = dn_dg + rhs.dn_dg;
		res.diagonal = diagonal + rhs.diagonal;
		for (uint32_t i = 0; i < Attr; ++i)
		{
			res.dm[i] = dm[i] + rhs.dm[i];
			res.gm[i] = gm[i] + rhs.gm[i];
		}
		res.n = n + rhs.n;
		res.d = d + rhs.d;
		return res;
	}

	KAttrQuadric operator-(const KAttrQuadric& rhs) const
	{
		KAttrQuadric res;
		res.nxn_gxg = nxn_gxg - rhs.nxn_gxg;
		res.d2_dm2 = d2_dm2 - rhs.d2_dm2;
		res.dn_dg = dn_dg - rhs.dn_dg;
		res.diagonal = diagonal - rhs.diagonal;
		for (uint32_t i = 0; i < Attr; ++i)
		{
			res.dm[i] = dm[i] - rhs.dm[i];
			res.gm[i] = gm[i] - rhs.gm[i];
		}
		res.n = n - rhs.n;
		res.d = d - rhs.d;
		return res;
	}

	KAttrQuadric& operator+=(const KAttrQuadric& rhs)
	{
		nxn_gxg += rhs.nxn_gxg;
		d2_dm2 += rhs.d2_dm2;
		dn_dg += rhs.dn_dg;
		diagonal += rhs.diagonal;
		for (uint32_t i = 0; i < Attr; ++i)
		{
			dm[i] += rhs.dm[i];
			gm[i] += rhs.gm[i];
		}
		n += rhs.n;
		d += rhs.d;
		return *this;
	}

	KAttrQuadric& operator-=(const KAttrQuadric& rhs)
	{
		nxn_gxg -= rhs.nxn_gxg;
		d2_dm2 -= rhs.d2_dm2;
		dn_dg -= rhs.dn_dg;
		diagonal -= rhs.diagonal;
		for (uint32_t i = 0; i < Attr; ++i)
		{
			dm[i] -= rhs.dm[i];
			gm[i] -= rhs.gm[i];
		}
		n -= rhs.n;
		d -= rhs.d;
		return *this;
	}

	KMatrix<T, Size, Size> ComputeA() const
	{
		KMatrix<T, Size, Size> A;
		for (uint32_t i = 0; i < 3; ++i)
		{
			for (uint32_t j = 0; j < 3; ++j)
			{
				A.m[i][j] = nxn_gxg.m[i][j];
			}
		}

		for (uint32_t k = 0; k < Attr; ++k)
		{
			A.m[k + 3][k + 3] = diagonal;
		}

		for (uint32_t i = 0; i < 3; ++i)
		{
			for (uint32_t k = 0; k < Attr; ++k)
			{
				A.m[i][k + 3] = -gm[k].v[i];
				A.m[k + 3][i] = -gm[k].v[i];
			}
		}

		return A;
	}

	KVector<T, Size> ComputeB() const
	{
		KVector<T, Size> b;

		for (uint32_t i = 0; i < 3; ++i)
		{
			b.v[i] = dn_dg.v[i];
		}

		for (uint32_t k = 0; k < Attr; ++k)
		{
			b.v[k + 3] = -dm[k];
		}

		return b;
	}

	T ComputeC() const
	{
		T c = d2_dm2;
		return c;
	}

	T Error(T* v) const
	{
		T error = 0;

		KMatrix<T, Size, Size> A = ComputeA();

		// vT * a * v
		for (uint32_t i = 0; i < Size; ++i)
		{
			for (uint32_t k = 0; k < Size; ++k)
			{
				error += v[k] * A.m[k][i] * v[i];
			}
		}

		KVector<T, Size> b = ComputeB();

		// 2 * bT * v
		for (uint32_t i = 0; i < Size; ++i)
		{
			error += 2 * b.v[i] * v[i];
		}

		// c
		error += ComputeC();

		return error;
	}

	bool OptimalVolume(T* x) const
	{
		if (diagonal < 1e-12f)
		{
			return false;
		}

		constexpr uint32_t m = Attr;

		KMatrix<T, 3, 3> C;
		KMatrix<T, 3, m> B;
		KMatrix<T, m, 3> BT;
		KVector<T, 3> b1;
		KVector<T, m> b2;

		C = nxn_gxg;
		for (uint32_t i = 0; i < 3; ++i)
		{
			for (uint32_t j = 0; j < m; ++j)
			{
				B.m[i][j] = -gm[j].v[i];
			}
		}
		BT = B.Transpose();

		KMatrix<T, 3, 3> _LHS = C - (Mul(B, BT) / diagonal);
		KMatrix<T, 4, 4> LHS;

		for (uint32_t i = 0; i < 3; ++i)
		{
			for (uint32_t j = 0; j < 3; ++j)
			{
				LHS.m[i][j] = _LHS.m[i][j];
			}
			LHS.m[i][3] = n.v[i];
			LHS.m[3][i] = n.v[i];
		}

		KMatrix<T, 4, 4> LU = LHS;

		uint32_t pivot[4] = { 0 };
		if (LUPFactorize(&LU.m[0][0], pivot, 4, (T)1e-12f))
		{
			b1 = -dn_dg;
			for (uint32_t i = 0; i < m; ++i)
			{
				b2.v[i] = dm[i];
			}

			KVector<T, 3> _RHS = b1 - (Mul(B, b2) / diagonal);

			KVector<T, 4> RHS;
			for (uint32_t i = 0; i < 3; ++i)
			{
				RHS.v[i] = _RHS.v[i];
			}
			RHS.v[3] = -d;

			KVector<T, 4> _pos;
			LUPSolveIterate(&LHS.m[0][0], &LU.m[0][0], pivot, 4, &RHS.v[0], &_pos.v[0]);
			// LUPSolve(&LHS.m[0][0], pivot, 4, &RHS.v[0], &_pos.v[0]);

			KVector<T, 3> pos;
			for (uint32_t i = 0; i < 3; ++i)
			{
				pos.v[i] = _pos.v[i];
			}

			KVector<T, m> attr;
			attr = (b2 - Mul(BT, pos)) / diagonal;

			for (uint32_t i = 0; i < 3; ++i)
			{
				x[i] = pos.v[i];
			}
			for (uint32_t i = 0; i < m; ++i)
			{
				x[3 + i] = attr.v[i];
			}

			return true;
		}
		else
		{
			return false;
		}
	}

	bool Optimal(T* x) const
	{
		if (diagonal < 1e-12f)
		{
			return false;
		}
#if 0
		// Solve a * x = -b
		uint32_t pivot[Size] = { 0 };

		KMatrix<T, Size, Size> A = -ComputeA();
		KMatrix<T, Size, Size> LU = A;

		if (LUPFactorize(&LU.m[0][0], pivot, Size, 1e-12f))
		{
			KVector<T, Size> b = ComputeB();
			LUPSolveIterate(&A.m[0][0], &LU.m[0][0], pivot, Size, &b.v[0], x);
			return true;
		}
		else
		{
			return false;
		}
#else
		constexpr uint32_t m = Attr;

		KMatrix<T, 3, 3> C;
		KMatrix<T, 3, m> B;
		KMatrix<T, m, 3> BT;
		KVector<T, 3> b1;
		KVector<T, m> b2;

		C = nxn_gxg;
		for (uint32_t i = 0; i < 3; ++i)
		{
			for (uint32_t j = 0; j < m; ++j)
			{
				B.m[i][j] = -gm[j].v[i];
			}
		}
		BT = B.Transpose();

		KMatrix<T, 3, 3> LHS = C - (Mul(B, BT) / diagonal);
		KMatrix<T, 3, 3> LU = LHS;

		uint32_t pivot[3] = { 0 };
		if (LUPFactorize(&LU.m[0][0], pivot, 3, (T)1e-12f))
		{
			b1 = -dn_dg;
			for (uint32_t i = 0; i < m; ++i)
			{
				b2.v[i] = dm[i];
			}

			KVector<T, 3> RHS = b1 - (Mul(B, b2) / diagonal);

			KVector<T, 3> pos;
			LUPSolveIterate(&LHS.m[0][0], &LU.m[0][0], pivot, 3, &RHS.v[0], &pos.v[0]);
			// LUPSolve(&LHS.m[0][0], pivot, 3, &RHS.v[0], &pos.v[0]);		

			KVector<T, m> attr;
			attr = (b2 - Mul(BT, pos)) / diagonal;

			for (uint32_t i = 0; i < 3; ++i)
			{
				x[i] = pos.v[i];
			}
			for (uint32_t i = 0; i < m; ++i)
			{
				x[3 + i] = attr.v[i];
			}

			return true;
		}
		else
		{
			return false;
		}
#endif
	}
};

class KMeshSimplification
{
protected:
	typedef float Type;

	constexpr static Type NORMAL_WEIGHT = 1;// 0.005f;
	constexpr static Type COLOR_WEIGHT = 0.5f;
	constexpr static Type UV_WEIGHT = 0.5f;

	struct InputVertexLayout
	{
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv;
	};

	struct Vertex
	{
		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 normal;
	};

	struct Triangle
	{
		int32_t index[3] = { -1, -1, -1 };

		int32_t PointIndex(int32_t v) const
		{
			for (int32_t i = 0; i < 3; ++i)
			{
				if (index[i] == v)
				{
					return i;
				}
			}
			return -1;
		}
	};

	struct Edge
	{
		int32_t index[2] = { -1, -1 };
	};

	struct EdgeContraction
	{
		Edge edge;
		Edge version;
		Type cost = 0;
		Vertex vertex;

		bool operator<(const EdgeContraction& rhs) const
		{
			return cost > rhs.cost;
		}
	};

	struct PointModify
	{
		int32_t triangleIndex = -1;
		int32_t pointIndex = -1;
		int32_t prevIndex = -1;
		int32_t currIndex = -1;
		std::vector<Triangle>* triangleArray = nullptr;

		void Redo()
		{
			std::vector<Triangle>& triangles = *triangleArray;
			assert(triangles[triangleIndex].index[pointIndex] == prevIndex);
			triangles[triangleIndex].index[pointIndex] = currIndex;
		}

		void Undo()
		{
			std::vector<Triangle>& triangles = *triangleArray;
			assert(triangles[triangleIndex].index[pointIndex] == currIndex);
			triangles[triangleIndex].index[pointIndex] = prevIndex;
		}
	};

	struct EdgeCollapse
	{
		std::vector<PointModify> modifies;

		int32_t prevTriangleCount = 0;
		int32_t prevVertexCount = 0;
		int32_t currTriangleCount = 0;
		int32_t currVertexCount = 0;

		int32_t* pCurrVertexCount = nullptr;
		int32_t* pCurrTriangleCount = nullptr;

		void Redo()
		{
			for (size_t i = 0; i < modifies.size(); ++i)
			{
				modifies[i].Redo();
			}

			assert(*pCurrVertexCount == prevVertexCount);
			assert(*pCurrTriangleCount == prevTriangleCount);
			*pCurrVertexCount = currVertexCount;
			*pCurrTriangleCount = currTriangleCount;
		}

		void Undo()
		{
			for (size_t i = 0; i < modifies.size(); ++i)
			{
				modifies[modifies.size() - 1 - i].Undo();
			}

			assert(*pCurrVertexCount == currVertexCount);
			assert(*pCurrTriangleCount == currTriangleCount);
			*pCurrVertexCount = prevVertexCount;
			*pCurrTriangleCount = prevTriangleCount;
		}
	};

	KAssetImportResult::Material m_Material;

	static constexpr uint32_t AttrNum = 5;
	typedef KAttrQuadric<Type, AttrNum> AtrrQuadric;

	static constexpr uint32_t QuadircDimension = AttrNum + 3;
	typedef KQuadric<Type, QuadircDimension> Quadric;
	typedef KVector<Type, QuadircDimension> Vector;
	typedef KMatrix<Type, QuadircDimension, QuadircDimension> Matrix;

	std::vector<Triangle> m_Triangles;
	std::vector<Vertex> m_Vertices;
	std::vector<std::unordered_set<int32_t>> m_Adjacencies;
	std::vector<int32_t> m_Versions;

	std::vector<Quadric> m_Quadric;
	std::vector<AtrrQuadric> m_AttrQuadric;

	std::vector<Quadric> m_TriQuadric;
	std::vector<AtrrQuadric> m_TriAttrQuadric;

	std::priority_queue<EdgeContraction> m_EdgeHeap;
	std::vector<EdgeCollapse> m_CollapseOperations;
	size_t m_CurrOpIdx = 0;

	int32_t m_CurVertexCount = 0;
	int32_t m_MinVertexCount = 0;
	int32_t m_MaxVertexCount = 0;

	int32_t m_CurTriangleCount = 0;
	int32_t m_MinTriangleCount = 0;
	int32_t m_MaxTriangleCount = 0;

	Type m_MaxErrorAllow = std::numeric_limits<Type>::max();
	int32_t m_MinTriangleAllow = 1;
	int32_t m_MinVertexAllow = 3;

	bool m_Memoryless = true;

	void UndoCollapse()
	{
		if (m_CurrOpIdx > 0)
		{
			--m_CurrOpIdx;
			EdgeCollapse collapse = m_CollapseOperations[m_CurrOpIdx];
			collapse.Undo();
		}
	}

	void RedoCollapse()
	{
		if (m_CurrOpIdx < m_CollapseOperations.size())
		{
			EdgeCollapse collapse = m_CollapseOperations[m_CurrOpIdx];
			collapse.Redo();
			++m_CurrOpIdx;
		}
	}

	bool IsDegenerateTriangle(const Triangle& triangle) const
	{
		int32_t v0 = triangle.index[0];
		int32_t v1 = triangle.index[1];
		int32_t v2 = triangle.index[2];

		if (v0 == v1)
			return false;
		if (v0 == v2)
			return false;
		if (v1 == v2)
			return false;

		const Vertex& vert0 = m_Vertices[v0];
		const Vertex& vert1 = m_Vertices[v1];
		const Vertex& vert2 = m_Vertices[v2];

		constexpr float EPS = 1e-3f;

		if (glm::length(vert0.pos - vert1.pos) < EPS)
			return true;
		if (glm::length(vert0.pos - vert1.pos) < EPS)
			return true;
		if (glm::length(vert1.pos - vert2.pos) < EPS)
			return true;

		return false;
	}

	bool IsValid(uint32_t triIndex) const
	{
		const Triangle& triangle = m_Triangles[triIndex];

		int32_t v0 = triangle.index[0];
		int32_t v1 = triangle.index[1];
		int32_t v2 = triangle.index[2];	

		if (v0 == v1)
			return false;
		if (v0 == v2)
			return false;
		if (v1 == v2)
			return false;

		return true;
	}

	std::tuple<Type, Vertex> ComputeCostAndVertex(const Edge& edge, const Quadric &quadric, const AtrrQuadric &attrQuadric)
	{
		Vertex vc;

		int32_t v0 = edge.index[0];
		int32_t v1 = edge.index[1];

		const Vertex& va = m_Vertices[v0];
		const Vertex& vb = m_Vertices[v1];

		glm::vec2 uvBox[2];

		for (uint32_t i = 0; i < 2; ++i)
		{
			uvBox[0][i] = std::min(va.uv[i], vb.uv[i]);
			uvBox[1][i] = std::max(va.uv[i], vb.uv[i]);
		}

		Type cost = std::numeric_limits<Type>::max();
		Vector opt, vec;

		if (attrQuadric.OptimalVolume(vec.v))
		{
			cost = attrQuadric.Error(vec.v);
			opt = vec;
		}
		else if (attrQuadric.Optimal(vec.v))
		{
			cost = attrQuadric.Error(vec.v);
			opt = vec;
		}
		else
		{
			constexpr size_t sgement = 3;
			static_assert(sgement >= 1, "ensure sgement");
			for (size_t i = 0; i < sgement; ++i)
			{
				glm::vec3 pos = glm::mix(va.pos, vb.pos, (float)(i) / (float)(sgement - 1));
				glm::vec2 uv = glm::mix(va.uv, vb.uv, (float)(i) / (float)(sgement - 1));
				glm::vec3 normal = glm::mix(va.normal, vb.normal, (float)(i) / (float)(sgement - 1));

				vec.v[0] = pos[0];		vec.v[1] = pos[1];		vec.v[2] = pos[2];
				vec.v[3] = uv[0];		vec.v[4] = uv[1];
				vec.v[5] = normal[0];	vec.v[6] = normal[1];	vec.v[7] = normal[2];

				Type thisCost = attrQuadric.Error(vec.v);
				if (thisCost < cost)
				{
					cost = thisCost;
					opt = vec;
				}
			}
		}

		vc.pos = glm::vec3(opt.v[0], opt.v[1], opt.v[2]);
		vc.uv = glm::clamp(glm::vec2(opt.v[3], opt.v[4]), uvBox[0], uvBox[1]);
		vc.normal = glm::normalize(glm::vec3(opt.v[5], opt.v[6], opt.v[7]));

		return std::make_tuple(cost, vc);
	};

	bool InitVertexData(const KAssetImportResult& input, size_t partIndex)
	{
		auto FindPNTIndex = [](const std::vector<KAssetVertexComponentGroup>& group) -> int32_t
		{
			for (int32_t i = 0; i < (int32_t)group.size(); ++i)
			{
				const KAssetVertexComponentGroup& componentGroup = group[i];
				if (componentGroup.size() == 3)
				{
					if (componentGroup[0] == AVC_POSITION_3F && componentGroup[1] == AVC_NORMAL_3F && componentGroup[2] == AVC_UV_2F)
					{
						return i;
					}
				}
			}
			return -1;
		};

		int32_t vertexDataIndex = FindPNTIndex(input.components);

		if (vertexDataIndex < 0)
		{
			return false;
		}

		if (partIndex < input.parts.size())
		{
			const KAssetImportResult::ModelPart& part = input.parts[partIndex];
			const KAssetImportResult::VertexDataBuffer& vertexData = input.verticesDatas[vertexDataIndex];

			m_Material = part.material;

			uint32_t indexBase = part.indexBase;
			uint32_t indexCount = part.indexCount;

			uint32_t vertexBase = part.vertexBase;
			uint32_t vertexCount = part.vertexCount;

			uint32_t indexMin = std::numeric_limits<uint32_t>::max();

			std::vector<uint32_t> indices;

			if (indexCount == 0)
			{
				return false;
			}
			else
			{
				indices.resize(indexCount);
				if (input.index16Bit)
				{
					const uint16_t* pIndices = (const uint16_t*)input.indicesData.data();
					pIndices += indexBase;
					for (uint32_t i = 0; i < indexCount; ++i)
					{
						indices[i] = pIndices[i];
					}
				}
				else
				{
					const uint32_t* pIndices = (const uint32_t*)input.indicesData.data();
					pIndices += indexBase;
					for (uint32_t i = 0; i < indexCount; ++i)
					{
						indices[i] = pIndices[i];
					}
				}
			}

			if (indexCount % 3 != 0)
			{
				return false;
			}

			for (uint32_t i = 0; i < indexCount; ++i)
			{
				if (indices[i] < indexMin)
				{
					indexMin = indices[i];
				}
			}

			for (uint32_t i = 0; i < indexCount; ++i)
			{
				indices[i] -= indexMin;
			}

			const InputVertexLayout* pVerticesData = (const InputVertexLayout*)vertexData.data();
			pVerticesData += vertexBase;

			m_Vertices.resize(vertexCount);
			m_Adjacencies.resize(vertexCount);
			m_Versions.resize(vertexCount);

			for (uint32_t i = 0; i < vertexCount; ++i)
			{
				const InputVertexLayout& srcVertex = pVerticesData[i];
				m_Vertices[i].pos = srcVertex.pos;
				m_Vertices[i].uv = srcVertex.uv;
				m_Vertices[i].normal = srcVertex.normal;
				m_Versions[i] = 0;
			}

			uint32_t maxTriCount = indexCount / 3;

			m_Triangles.reserve(maxTriCount);

			for (uint32_t i = 0; i < maxTriCount; ++i)
			{
				Triangle triangle;
				triangle.index[0] = indices[3 * i];
				triangle.index[1] = indices[3 * i + 1];
				triangle.index[2] = indices[3 * i + 2];
				if (!IsDegenerateTriangle(triangle))
				{
					for (uint32_t i = 0; i < 3; ++i)
					{
						assert(triangle.index[i] < m_Adjacencies.size());
						m_Adjacencies[triangle.index[i]].insert((int32_t)(m_Triangles.size()));
					}
					m_Triangles.push_back(triangle);
				}
			}

			m_MaxTriangleCount = (int32_t)m_Triangles.size();
			m_MaxVertexCount = 0;

			for (uint32_t i = 0; i < vertexCount; ++i)
			{
				if (m_Adjacencies[i].size() != 0)
				{
					++m_MaxVertexCount;
				}
			}

			return true;
		}
		return false;
	}

	Quadric ComputeQuadric(const Triangle& triangle)
	{
		const Vertex& va = m_Vertices[triangle.index[0]];
		const Vertex& vb = m_Vertices[triangle.index[1]];
		const Vertex& vc = m_Vertices[triangle.index[2]];

		Quadric res;

		Vector p, q, r;
		p.v[0] = va.pos[0]; p.v[1] = va.pos[1]; p.v[2] = va.pos[2];
		q.v[0] = vb.pos[0]; q.v[1] = vb.pos[1]; q.v[2] = vb.pos[2];
		r.v[0] = vc.pos[0]; r.v[1] = vc.pos[1]; r.v[2] = vc.pos[2];

		p.v[3] = va.uv[0]; p.v[4] = va.uv[1];
		q.v[3] = vb.uv[0]; q.v[4] = vb.uv[1];
		r.v[3] = vc.uv[0]; r.v[4] = vc.uv[1];

		p.v[5] = va.normal[0]; p.v[6] = va.normal[1]; p.v[7] = va.normal[2];
		q.v[5] = vb.normal[0]; q.v[6] = vb.normal[1]; q.v[7] = vb.normal[2];
		r.v[5] = vc.normal[0]; r.v[6] = vc.normal[1]; r.v[7] = vc.normal[2];

		Vector e1 = q - p;
		e1 /= e1.Length();

		Vector e2 = r - p;
		e2 -= e1 * e2.Dot(e1);
		e2 /= e2.Length();

		Matrix A = Matrix(1.0f) - Mul(e1, e1) - Mul(e2, e2);
		Vector b = e1 * e1.Dot(p) + e2 * e2.Dot(p) - p;
		Type c = p.Dot(p) - (p.Dot(e1) * p.Dot(e1)) - (p.Dot(e2) * p.Dot(e2));

		for (uint32_t i = 0; i < 3; ++i)
		{
			for (uint32_t j = i; j < 3; ++j)
			{
				res.GetA(i, j) = A.m[i][j];
			}
			res.b[i] = b.v[i];
		}

		res.c = c;

		glm::vec3 n = glm::cross(vb.pos - va.pos, vc.pos - va.pos);
		Type area = 0.5f * glm::length(n);
		res *= area;

		return res;
	};

	AtrrQuadric ComputeAttrQuadric(const Triangle& triangle)
	{
		const Vertex& va = m_Vertices[triangle.index[0]];
		const Vertex& vb = m_Vertices[triangle.index[1]];
		const Vertex& vc = m_Vertices[triangle.index[2]];

		Type m[AttrNum * 3];

		const Vertex* v[3] = { &va, &vb, &vc };

		for (uint32_t i = 0; i < 3; ++i)
		{
			m[i * AttrNum + 0] = UV_WEIGHT * (*v[i]).uv[0];
			m[i * AttrNum + 1] = UV_WEIGHT * (*v[i]).uv[1];
			m[i * AttrNum + 2] = NORMAL_WEIGHT * (*v[i]).normal[0];
			m[i * AttrNum + 3] = NORMAL_WEIGHT * (*v[i]).normal[1];
			m[i * AttrNum + 4] = NORMAL_WEIGHT * (*v[i]).normal[2];
		}

		KVector<Type, 3> p, q, r;
		p.v[0] = va.pos[0]; p.v[1] = va.pos[1]; p.v[2] = va.pos[2];
		q.v[0] = vb.pos[0]; q.v[1] = vb.pos[1]; q.v[2] = vb.pos[2];
		r.v[0] = vc.pos[0]; r.v[1] = vc.pos[1]; r.v[2] = vc.pos[2];

		AtrrQuadric res = AtrrQuadric(p, q, r, m);

		glm::vec3 n = glm::cross(vb.pos - va.pos, vc.pos - va.pos);
		Type area = 0.5f * glm::length(n);
		res *= area;

		return res;
	};

	bool InitHeapData()
	{
		m_Quadric.resize(m_Vertices.size());
		m_AttrQuadric.resize(m_Vertices.size());

		m_TriQuadric.resize(m_Triangles.size());
		m_TriAttrQuadric.resize(m_Triangles.size());

		for (size_t triIndex = 0; triIndex < m_Triangles.size(); ++triIndex)
		{
			m_TriQuadric[triIndex] = ComputeQuadric(m_Triangles[triIndex]);
			m_TriAttrQuadric[triIndex] = ComputeAttrQuadric(m_Triangles[triIndex]);
		}

		for (size_t vertIndex = 0; vertIndex < m_Adjacencies.size(); ++vertIndex)
		{
			m_Quadric[vertIndex] = Quadric();
			m_AttrQuadric[vertIndex] = AtrrQuadric();
			for (int32_t triIndex : m_Adjacencies[vertIndex])
			{
				m_Quadric[vertIndex] += m_TriQuadric[triIndex];
				m_AttrQuadric[vertIndex] += m_TriAttrQuadric[triIndex];
			}
		}

		for (size_t triIndex = 0; triIndex < m_Triangles.size(); ++triIndex)
		{
			const Triangle& triangle = m_Triangles[triIndex];
			for (size_t i = 0; i < 3; ++i)
			{
				EdgeContraction contraction;
				int32_t v0 = triangle.index[i];
				int32_t v1 = triangle.index[(i + 1) % 3];
				contraction.edge.index[0] = v0;
				contraction.edge.index[1] = v1;
				contraction.version.index[0] = m_Versions[v0];
				contraction.version.index[1] = m_Versions[v1];
				std::tuple<Type, Vertex> costAndVertex = ComputeCostAndVertex(contraction.edge, m_Quadric[v0] + m_Quadric[v1], m_AttrQuadric[v0] + m_AttrQuadric[v1]);
				contraction.cost = std::get<0>(costAndVertex);
				contraction.vertex = std::get<1>(costAndVertex);
				m_EdgeHeap.push(contraction);
			}
		}

		if (m_Memoryless)
		{
			m_Quadric.clear();
			m_AttrQuadric.clear();
		}

		return true;
	}

	bool PerformSimplification()
	{
		m_CurVertexCount = m_MinVertexCount = m_MaxVertexCount;
		m_CurTriangleCount = m_MinTriangleCount = m_MaxTriangleCount;

		size_t performCounter = 0;

		auto CheckValidFlag = [this](const Triangle& triangle)
		{
			if (m_Versions[triangle.index[0]] < 0)
				return false;
			if (m_Versions[triangle.index[1]] < 0)
				return false;
			if (m_Versions[triangle.index[2]] < 0)
				return false;
			return true;
		};

		auto CheckEdge = [this](int32_t vertIndex)
		{
			std::set<int32_t> adjacencies;
			for (int32_t triIndex : m_Adjacencies[vertIndex])
			{
				if (IsValid(triIndex))
				{
					int32_t index = m_Triangles[triIndex].PointIndex(vertIndex);
					assert(index >= 0);
					adjacencies.insert(m_Triangles[triIndex].index[(index + 1) % 3]);
					adjacencies.insert(m_Triangles[triIndex].index[(index + 2) % 3]);
				}
			}
			for (int32_t adjIndex : adjacencies)
			{
				std::set<int32_t> tris;
				for (int32_t triIndex : m_Adjacencies[vertIndex])
				{
					if (IsValid(triIndex))
					{
						if (m_Adjacencies[adjIndex].find(triIndex) != m_Adjacencies[adjIndex].end())
						{
							tris.insert(triIndex);
						}
					}
				}
				if (tris.size() > 2)
				{
					for (int32_t triIndex : tris)
					{
						printf("[%d] %d,%d,%d\n", triIndex, m_Triangles[triIndex].index[0], m_Triangles[triIndex].index[1], m_Triangles[triIndex].index[2]);
					}
					return false;
				}
			}
			return true;
		};

		while (!m_EdgeHeap.empty())
		{
			if (m_CurVertexCount < m_MinVertexAllow)
				break;
			if (m_CurTriangleCount < m_MinTriangleAllow)
				break;

			EdgeContraction contraction;
			bool validContraction = false;

			do
			{
				contraction = m_EdgeHeap.top();
				m_EdgeHeap.pop();
				validContraction = (m_Versions[contraction.edge.index[0]] == contraction.version.index[0] && m_Versions[contraction.edge.index[1]] == contraction.version.index[1]);
			} while (!m_EdgeHeap.empty() && !validContraction);

			if (!validContraction)
			{
				break;
			}

			if (contraction.cost > m_MaxErrorAllow)
				break;

			assert(contraction.edge.index[0] != contraction.edge.index[1]);

			int32_t v0 = contraction.edge.index[0];
			int32_t v1 = contraction.edge.index[1];

			std::unordered_set<int32_t> sharedAdjacencySet;
			std::unordered_set<int32_t> noSharedAdjacencySetV0;
			std::unordered_set<int32_t> noSharedAdjacencySetV1;

			for (int32_t triIndex : m_Adjacencies[v1])
			{
				if (IsValid(triIndex))
				{
					assert(CheckValidFlag(m_Triangles[triIndex]));
					if (m_Adjacencies[v0].find(triIndex) != m_Adjacencies[v0].end())
					{
						sharedAdjacencySet.insert(triIndex);
					}
					else
					{
						noSharedAdjacencySetV1.insert(triIndex);
					}
				}
			}

			for (int32_t triIndex : m_Adjacencies[v0])
			{
				if (IsValid(triIndex))
				{
					assert(CheckValidFlag(m_Triangles[triIndex]));
					if (sharedAdjacencySet.find(triIndex) == sharedAdjacencySet.end())
					{
						noSharedAdjacencySetV0.insert(triIndex);
					}
				}
			}

			int32_t invalidTriangle = (int32_t)sharedAdjacencySet.size();
			if (m_CurTriangleCount - invalidTriangle < m_MinTriangleAllow)
			{
				continue;
			}

			std::unordered_set<int32_t> sharedVerts;
			for (int32_t triIndex : sharedAdjacencySet)
			{
				Triangle& triangle = m_Triangles[triIndex];
				for (int32_t vertId : triangle.index)
				{
					if (vertId != v0 && vertId != v1)
					{
						sharedVerts.insert(vertId);
					}
				}
			}

			/*
				<<Polygon Mesh Processing>> 7. Simplification & Approximation
				1. If both p and q are boundary vertices, then the edge (p, q) has to be a boundary edge.
				2. For all vertices r incident to both p and q there has to be a triangle(p, q, r).
				In other words, the intersection of the one-rings of p and q consists of vertices opposite the edge (p, q) only.
			*/
			auto HasIndirectConnect = [this](int32_t v0, int32_t v1, const std::unordered_set<int32_t>& noSharedAdjacencySetV0, const std::unordered_set<int32_t>& sharedAdjacencySet, std::unordered_set<int32_t>& sharedVerts) -> bool
			{
				std::set<int32_t> noSharedAdjVerts;
				for (int32_t triIndex : noSharedAdjacencySetV0)
				{
					int32_t index = m_Triangles[triIndex].PointIndex(v0);
					assert(index >= 0);
					int32_t va = m_Triangles[triIndex].index[(index + 1) % 3];
					int32_t vb = m_Triangles[triIndex].index[(index + 2) % 3];
					if (sharedVerts.find(va) == sharedVerts.end())
					{
						noSharedAdjVerts.insert(va);
					}
					if (sharedVerts.find(vb) == sharedVerts.end())
					{
						noSharedAdjVerts.insert(vb);
					}
				}
				for (int32_t adjVert : noSharedAdjVerts)
				{
					std::set<int32_t> tris;
					for (int32_t triIndex : m_Adjacencies[adjVert])
					{
						if (IsValid(triIndex))
						{
							if (sharedAdjacencySet.find(triIndex) != sharedAdjacencySet.end())
							{
								continue;
							}
							int32_t index = m_Triangles[triIndex].PointIndex(v1);
							if (index >= 0)
							{
								return true;
							}
						}
					}
				}
				return false;
			};

			if (HasIndirectConnect(v0, v1, noSharedAdjacencySetV0, sharedAdjacencySet, sharedVerts))
			{
				continue;
			}

			auto TriangleWillInvert = [this, &contraction](int32_t v, const std::unordered_set<int32_t>& noSharedAdjacencySet)
			{
				for (int32_t triIndex : noSharedAdjacencySet)
				{
					const Triangle& triangle = m_Triangles[triIndex];
					int32_t i = triangle.PointIndex(v);

					glm::vec3 newNormal = glm::cross(
						m_Vertices[triangle.index[(i + 1) % 3]].pos - contraction.vertex.pos,
						m_Vertices[triangle.index[(i + 2) % 3]].pos - contraction.vertex.pos);

					glm::vec3 oldNormal = glm::cross(
						m_Vertices[triangle.index[(i + 1) % 3]].pos - m_Vertices[triangle.index[i]].pos,
						m_Vertices[triangle.index[(i + 2) % 3]].pos - m_Vertices[triangle.index[i]].pos);

					if (glm::dot(newNormal, oldNormal) < 0)
					{
						return true;
					}
				}
				return false;
			};

			if (TriangleWillInvert(v0, noSharedAdjacencySetV0))
			{
				continue;
			}

			if (TriangleWillInvert(v1, noSharedAdjacencySetV1))
			{
				continue;
			}

			++performCounter;

			int32_t newIndex = (int32_t)m_Vertices.size();

			m_Vertices.push_back(contraction.vertex);
			m_Adjacencies.push_back({});
			m_Versions.push_back(0);

			std::unordered_set<int32_t> adjacencyVert;

			if (m_Memoryless)
			{
				for (int32_t triIndex : noSharedAdjacencySetV0)
				{
					Triangle triangle = m_Triangles[triIndex];
					int32_t idx = triangle.PointIndex(v0);
					assert(idx >= 0);
					triangle.index[idx] = newIndex;

					m_TriQuadric[triIndex] = ComputeQuadric(triangle);
					m_TriAttrQuadric[triIndex] = ComputeAttrQuadric(triangle);

					adjacencyVert.insert(triangle.index[(idx + 1) % 3]);
					adjacencyVert.insert(triangle.index[(idx + 2) % 3]);
				}

				for (int32_t triIndex : noSharedAdjacencySetV1)
				{
					Triangle triangle = m_Triangles[triIndex];
					int32_t idx = triangle.PointIndex(v1);
					assert(idx >= 0);
					triangle.index[idx] = newIndex;

					m_TriQuadric[triIndex] = ComputeQuadric(triangle);
					m_TriAttrQuadric[triIndex] = ComputeAttrQuadric(triangle);

					adjacencyVert.insert(triangle.index[(idx + 1) % 3]);
					adjacencyVert.insert(triangle.index[(idx + 2) % 3]);
				}
			}
			else
			{
				m_Quadric.push_back(m_Quadric[v0] + m_Quadric[v1]);
				m_AttrQuadric.push_back(m_AttrQuadric[v0] + m_AttrQuadric[v1]);
			}

			auto NewModify = [this, newIndex](int32_t triIndex, int32_t pointIndex)->PointModify
			{
				PointModify modify;
				modify.triangleIndex = triIndex;
				modify.pointIndex = pointIndex;
				modify.triangleArray = &m_Triangles;
				return modify;
			};

			auto NewContraction = [this, newIndex](const Triangle& triangle, int32_t i, int32_t j)
			{
				EdgeContraction contraction;

				int32_t v0 = triangle.index[i];
				int32_t v1 = triangle.index[j];
				contraction.edge.index[0] = v0;
				contraction.edge.index[1] = v1;
				assert(contraction.edge.index[0] != contraction.edge.index[1]);
				contraction.version.index[0] = m_Versions[v0];
				contraction.version.index[1] = m_Versions[v1];

				std::tuple<Type, Vertex> costAndVertex;
				if (m_Memoryless)
				{
					Quadric quadric;
					AtrrQuadric atrrQuadric;

					for (int32_t triIndex : m_Adjacencies[v0])
					{
						if (IsValid(triIndex))
						{
							quadric += m_TriQuadric[triIndex];
							atrrQuadric += m_TriAttrQuadric[triIndex];
						}
					}

					for (int32_t triIndex : m_Adjacencies[v1])
					{
						if (IsValid(triIndex))
						{
							quadric += m_TriQuadric[triIndex];
							atrrQuadric += m_TriAttrQuadric[triIndex];
						}
					}

					costAndVertex = ComputeCostAndVertex(contraction.edge, quadric, atrrQuadric);
				}
				else
				{
					costAndVertex = ComputeCostAndVertex(contraction.edge, m_Quadric[v0] + m_Quadric[v1], m_AttrQuadric[v0] + m_AttrQuadric[v1]);
				}

				contraction.cost = std::get<0>(costAndVertex);
				contraction.vertex = std::get<1>(costAndVertex);

				m_EdgeHeap.push(contraction);
			};

			EdgeCollapse collapse;
			collapse.pCurrTriangleCount = &m_CurTriangleCount;
			collapse.pCurrVertexCount = &m_CurVertexCount;

			std::unordered_set<int32_t> newAdjacencySet;

			auto AdjustAdjacencies = [this, newIndex, NewModify, CheckValidFlag, &sharedAdjacencySet, &newAdjacencySet, &collapse](int32_t v)
			{
				for (int32_t triIndex : m_Adjacencies[v])
				{
					Triangle& triangle = m_Triangles[triIndex];

					int32_t i = triangle.PointIndex(v);
					assert(i >= 0);
					triangle.index[i] = newIndex;

					PointModify modify = NewModify(triIndex, i);
					modify.prevIndex = v;
					modify.currIndex = newIndex;
					assert(modify.prevIndex != modify.currIndex);
					collapse.modifies.push_back(modify);

					if (IsValid(triIndex))
					{
						// See sharedVerts
						if (!CheckValidFlag(triangle))
						{
							continue;
						}
						if (sharedAdjacencySet.find(triIndex) != sharedAdjacencySet.end())
						{
							continue;
						}
						newAdjacencySet.insert(triIndex);
					}
				}
			};

			auto BuildNewContraction = [this, NewContraction, CheckValidFlag](int32_t v)
			{
				for (int32_t triIndex : m_Adjacencies[v])
				{
					const Triangle& triangle = m_Triangles[triIndex];
					if (!IsValid(triIndex))
					{
						continue;
					}
					if (!CheckValidFlag(triangle))
					{
						continue;
					}
					int32_t i = triangle.PointIndex(v);
					assert(i >= 0);

					NewContraction(triangle, i, (i + 1) % 3);
					NewContraction(triangle, i, (i + 2) % 3);
				}
			};

			int32_t invalidVertex = 0;
			for (int32_t vertId : sharedVerts)
			{
				assert(m_Versions[vertId] >= 0);
				bool hasValidTri = false;
				for (int32_t triIndex : m_Adjacencies[vertId])
				{
					if (IsValid(triIndex))
					{
						assert(CheckValidFlag(m_Triangles[triIndex]));
						if (sharedAdjacencySet.find(triIndex) != sharedAdjacencySet.end())
						{
							continue;
						}
						hasValidTri = true;
						break;
					}
				}
				if (!hasValidTri)
				{
					m_Versions[vertId] = -1;
					++invalidVertex;
				}
			}

			collapse.prevTriangleCount = m_CurTriangleCount;
			collapse.prevVertexCount = m_CurVertexCount;

			AdjustAdjacencies(v0);
			AdjustAdjacencies(v1);

			m_Adjacencies[newIndex] = newAdjacencySet;

			m_Versions[v0] = m_Versions[v1] = -1;
			if (newAdjacencySet.size() == 0)
			{
				m_Versions[newIndex] = -1;
				invalidVertex += 1;
			}

			if (m_Memoryless)
			{
				for (int32_t vertId : adjacencyVert)
				{
					++m_Versions[vertId];
				}
				for (int32_t vertId : adjacencyVert)
				{
					BuildNewContraction(vertId);
				}
			}
			else
			{
				BuildNewContraction(newIndex);
			}

			m_CurTriangleCount -= invalidTriangle;
			m_CurVertexCount -= invalidVertex + 1;

			collapse.currTriangleCount = m_CurTriangleCount;
			collapse.currVertexCount = m_CurVertexCount;

			if (!collapse.modifies.empty())
			{
				m_CollapseOperations.push_back(collapse);
				++m_CurrOpIdx;
			}

			/*
			if (!CheckEdge(newIndex))
			{
				m_Vertices[v0].uv = glm::vec2(0.2f, 0.05f);
				m_Vertices[v1].uv = glm::vec2(0.2f, 0.05f);
				break;
			}
			*/
		}

		m_MinTriangleCount = m_CurTriangleCount;
		m_MinVertexCount = m_CurVertexCount;

		return true;
	}
public:
	bool Init(const KAssetImportResult& input, size_t partIndex)
	{
		UnInit();
		if (InitVertexData(input, partIndex) && InitHeapData())
		{
			if (PerformSimplification())
				return true;
		}
		UnInit();
		return false;
	}

	bool UnInit()
	{
		m_Triangles.clear();
		m_Vertices.clear();
		m_Quadric.clear();
		m_AttrQuadric.clear();
		m_TriQuadric.clear();
		m_TriAttrQuadric.clear();
		m_EdgeHeap = std::priority_queue<EdgeContraction>();
		m_Adjacencies.clear();
		m_Versions.clear();
		m_CollapseOperations.clear();
		m_CurrOpIdx = 0;
		m_CurVertexCount = 0;
		m_MinVertexCount = 0;
		m_MaxVertexCount = 0;
		m_CurTriangleCount = 0;
		m_MinTriangleCount = 0;
		m_MaxTriangleCount = 0;
		return true;
	}

	inline int32_t& GetMinVertexCount() { return m_MinVertexCount; }
	inline int32_t& GetMaxVertexCount() { return m_MaxVertexCount; }
	inline int32_t& GetCurVertexCount() { return m_CurVertexCount; }

	bool Simplification(int32_t targetCount, KAssetImportResult& output)
	{
		if (targetCount >= m_MinVertexCount && targetCount <= m_MaxVertexCount)
		{
			while (m_CurVertexCount > targetCount)
			{
				RedoCollapse();
			}

			while (m_CurVertexCount < targetCount)
			{
				UndoCollapse();
			}

			std::vector<uint32_t> indices;
			for (uint32_t triIndex = 0; triIndex < (uint32_t)m_Triangles.size(); ++triIndex)
			{
				if (IsValid(triIndex))
				{
					indices.push_back(m_Triangles[triIndex].index[0]);
					indices.push_back(m_Triangles[triIndex].index[1]);
					indices.push_back(m_Triangles[triIndex].index[2]);
				}
			}
			if (indices.size() == 0)
			{
				return false;
			}

			std::unordered_map<uint32_t, uint32_t> remapIndices;
			std::vector<Vertex> vertices;

			for (size_t i = 0; i < indices.size(); ++i)
			{
				uint32_t oldIndex = indices[i];
				auto it = remapIndices.find(oldIndex);
				if (it == remapIndices.end())
				{
					int32_t mapIndex = (int32_t)remapIndices.size();
					remapIndices.insert({ oldIndex, mapIndex });
					vertices.push_back(m_Vertices[oldIndex]);
				}
			}

			for (size_t i = 0; i < indices.size(); ++i)
			{
				indices[i] = remapIndices[indices[i]];
			}

			KAssetImportResult::ModelPart part;
			part.indexBase = 0;
			part.indexCount = (uint32_t)indices.size();
			part.vertexBase = 0;
			part.vertexCount = (uint32_t)vertices.size();
			part.material = m_Material;

			std::vector<InputVertexLayout> outputVertices;
			outputVertices.resize(vertices.size());

			KAABBBox bound;

			for (size_t i = 0; i < vertices.size(); ++i)
			{
				outputVertices[i].pos = vertices[i].pos;
				outputVertices[i].normal = vertices[i].normal;
				outputVertices[i].uv = vertices[i].uv;
				bound.Merge(vertices[i].pos, bound);
			}

			output.components = { { AVC_POSITION_3F, AVC_NORMAL_3F, AVC_UV_2F } };
			output.parts = { part };
			output.vertexCount = (uint32_t)vertices.size();

			output.index16Bit = false;
			output.indicesData.resize(sizeof(indices[0]) * indices.size());
			memcpy(output.indicesData.data(), indices.data(), output.indicesData.size());

			KAssetImportResult::VertexDataBuffer vertexBuffer;
			vertexBuffer.resize(sizeof(outputVertices[0]) * outputVertices.size());
			memcpy(vertexBuffer.data(), outputVertices.data(), vertexBuffer.size());

			output.verticesDatas = { vertexBuffer };

			output.extend.min[0] = bound.GetMin()[0];
			output.extend.min[1] = bound.GetMin()[1];
			output.extend.min[2] = bound.GetMin()[2];

			output.extend.max[0] = bound.GetMax()[0];
			output.extend.max[1] = bound.GetMax()[1];
			output.extend.max[2] = bound.GetMax()[2];

			return true;
		}
		return false;
	}
};
#pragma once

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
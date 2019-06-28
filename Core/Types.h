#pragma once
#include "Core_Config.h"
#include <math.h>
#include <Eigen>
namespace rsdn
{
	typedef Eigen::Vector2f Point2;
	typedef Eigen::Vector3f Vector3;
	typedef Eigen::Vector3f Point3;
	typedef Eigen::Vector4f Vector4;
	typedef Eigen::Matrix3f Matrix3;

	template<typename Derived>
	inline bool is_nan(const Eigen::MatrixBase<Derived>& x)
	{
		return ((x.array() == x.array())).all();
	}

	template<typename Derived>
	inline bool has_nan(const Eigen::MatrixBase<Derived>& x)
	{
		return !((x.array() == x.array())).any();
	}

	class AngleCalculator
	{
	public:
		template<typename Derived>
		inline static datatype AngleBetween(const Eigen::MatrixBase<Derived>& vector1, const Eigen::MatrixBase<Derived>& vector2)
		{
			auto v1 = vector1.normalized();
			auto v2 = vector2.normalized();
			datatype num = v1.dot(v2);
			datatype radians;
			if (num < 0.f)
				radians = M_PI - 2.f * asin((-vector1 - vector2).norm() / 2.f);
			else
				radians = 2.f * asin((vector1 - vector2).norm() / 2.f);
			return radians * 180.f / M_PI;
		}

		template<typename T>
		inline static T GetAngleBetween3Points(T x1, T y1, T x2, T y2, T x3, T y3)
		{
			T vx1 = x2 - x1;
			T vy1 = y2 - y1;
			T vx2 = x2 - x3;
			T vy2 = y2 - y3;

			T y = vx1 * vy2 - vx2 * vy1;
			T x = vx1 * vx2 + vy1 * vy2;
			T angle = atan2(y, x) / M_PI * 180.0;
			if (angle < 0.0) angle += 360.0;
			return angle;
		}

		template<typename T>
		inline static T GetAngleBetweenPoints(T x1, T y1, T z1, T x2, T y2, T z2)
		{
			T dot = x1 * x2 + y1 * y2 + z1 * z2;
			T cos_theta = dot / (sqrt(x1 * x1 + y1 * y1 + z1 * z1) * sqrt(x2 * x2 + y2 * y2 + z2 * z2));

			return acos(cos_theta) / M_PI * 180.0;
		}

		template<typename T>
		inline static T Process(T x1, T y1, T z1, T x2, T y2, T z2, T x3, T y3, T z3, bool supplementary = false, bool relative = false)
		{
			T rx1 = x1 - x2;
			T ry1 = y1 - y2;
			T rz1 = z1 - z2;
			T rx2 = x3 - x2;
			T ry2 = y3 - y2;
			T rz2 = z3 - z2;
			T angle = GetAngleBetweenPoints(rx1, ry1, rz1, rx2, ry2, rz2);

			if (supplementary)
			{
				if (!relative)
				{
					return angle > 180.0 ? angle - 180.0 : 180.0 - angle;
				}
				else
				{
					return 180.0 - angle;
				}
			}

			if (relative)
			{
				return angle > 180.0 ? 360.0 - angle : -angle;
			}
			return angle > 180.0 ? 360.0 - angle : angle;
		}

		template<typename Derived>
		inline static datatype Process(const Eigen::MatrixBase<Derived>& md1, const Eigen::MatrixBase<Derived>& md2, const Eigen::MatrixBase<Derived>& md3, bool supplementary = false, bool relative = false)
		{
			return Process<datatype>(md1.x(), md1.y(), md1.z(), md2.x(), md2.y(), md2.z(), md3.x(), md3.y(), md3.z(), supplementary, relative);
		}

	};


	template<typename T>
	class Table
	{
	};

	template<>
	class Table<datatype>
	{
	private:
		bool _keepmemory;
		size_t _length;
		datatype* _raw;
		datatype** _rows;
	public:
		Table(size_t len) : _keepmemory(false), _length(len)
		{
			_raw = (datatype*)malloc(sizeof(datatype) * len * len);
			_rows = (datatype**)malloc(sizeof(datatype*) *len);
			datatype* tmpraw = _raw;
			for (size_t i = 0; i < len; i++)
			{
				_rows[i] = tmpraw;
				tmpraw += len;
			}

			Clear();
		}

		Table(datatype* prevraw, datatype** prevrows, size_t len) :_raw(prevraw), _length(len), _rows(prevrows), _keepmemory(false)
		{

		}

		Table(Table&& other)
		{
			_length = other._length;
			_raw = other._raw;
			_rows = other._rows;
			_keepmemory = other._keepmemory;
		}

		void unsafe_KeepLifeCycle()
		{
			_keepmemory = true;
		}

		~Table()
		{
			if (_keepmemory) return;
			if (_raw) { free(_raw); _raw = nullptr; }
			if (_rows) { free(_rows); _rows = nullptr; }
		}

		datatype* AddressOf()
		{
			return _raw;
		}

		datatype* At(size_t row)
		{
			return _rows[row];
		}

		datatype At(size_t row, size_t col) const
		{
			return _rows[row][col];
		}

		datatype& At(size_t row, size_t col)
		{
			return _rows[row][col];
		}

		operator bool() const
		{
			return _raw;
		}

		void Clear()
		{
			size_t len = _length*_length;
			for (size_t i = 0; i < len; i++) _raw[i] = 0.0f;
		}
	};

	template<>
	class Table<int>
	{
	private:
		bool _keepmemory;
		size_t _length;
		int* _raw;
		int** _rows;
	public:
		Table(size_t len) : _keepmemory(false), _length(len)
		{
			_raw = (int*)malloc(sizeof(int) * len * len);
			_rows = (int**)malloc(sizeof(int*) *len);
			int* tmpraw = _raw;
			for (size_t i = 0; i < len; i++)
			{
				_rows[i] = tmpraw;
				tmpraw += len;
			}

			Clear();
		}

		Table(int* prevraw, int** prevrows, size_t len) :_raw(prevraw), _length(len), _rows(prevrows), _keepmemory(false)
		{

		}

		Table(Table&& other)
		{
			_length = other._length;
			_raw = other._raw;
			_rows = other._rows;
			_keepmemory = other._keepmemory;
		}

		void unsafe_KeepLifeCycle()
		{
			_keepmemory = true;
		}

		~Table()
		{
			if (_keepmemory) return;
			if (_raw) { free(_raw); _raw = nullptr; }
			if (_rows) { free(_rows); _rows = nullptr; }
		}

		int* AddressOf() const
		{
			return _raw;
		}

		int* At(size_t row) const
		{
			return _rows[row];
		}

		int At(size_t row, size_t col) const
		{
			return _rows[row][col];
		}

		int& At(size_t row, size_t col)
		{
			return _rows[row][col];
		}

		operator bool() const
		{
			return _raw;
		}

		void Clear()
		{
			memset(_raw, 0, sizeof(int) * _length * _length);
		}
	};
}
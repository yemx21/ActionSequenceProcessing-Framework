#pragma once
#include "STRUCT_Config.h"
#include <cstdio>
#include <cassert>
#include <vector>
#include <algorithm>

namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			namespace operators
			{
				namespace details
				{
					class band_matrix
					{
					private:
						std::vector< std::vector<datatype> > m_upper; 
						std::vector< std::vector<datatype> > m_lower;
					public:
						band_matrix() {};                             
						band_matrix(int dim, int n_u, int n_l);       
						~band_matrix() {};                          
						void resize(int dim, int n_u, int n_l);    
						int dim() const;                             
						int num_upper() const
						{
							return m_upper.size() - 1;
						}
						int num_lower() const
						{
							return m_lower.size() - 1;
						}
	
						datatype & operator () (int i, int j);     
						datatype   operator () (int i, int j) const;    
																	
						datatype& saved_diag(int i);
						datatype  saved_diag(int i) const;
						void lu_decompose();
						std::vector<datatype> r_solve(const std::vector<datatype>& b) const;
						std::vector<datatype> l_solve(const std::vector<datatype>& b) const;
						std::vector<datatype> lu_solve(const std::vector<datatype>& b,
							bool is_lu_decomposed = false);

					};


					class Spline
					{
					public:
						enum bd_type {
							first_deriv = 1,
							second_deriv = 2
						};

					private:
						std::vector<datatype> m_x, m_y;         
						// interpolation parameters
						// f(x) = a*(x-x_i)^3 + b*(x-x_i)^2 + c*(x-x_i) + y_i
						std::vector<datatype> m_a, m_b, m_c;        // spline coefficients
						datatype  m_b0, m_c0;                     // for left extrapol
						bd_type m_left, m_right;
						datatype  m_left_value, m_right_value;
						bool    m_force_linear_extrapolation;

					public:
						Spline() : m_left(second_deriv), m_right(second_deriv),
							m_left_value(0.0), m_right_value(0.0),
							m_force_linear_extrapolation(false)
						{
							;
						}

						void set_boundary(bd_type left, datatype left_value,
							bd_type right, datatype right_value,
							bool force_linear_extrapolation = false);
						void set_points(const std::vector<datatype>& x,
							const std::vector<datatype>& y, bool cubic_spline = true);
						bool set_points1(const datatype* x,
							const datatype* y, size_t len, bool filternan=true, bool cubic_spline = true);

						void interp_points(const datatype* x,
							datatype* y, size_t len);

						datatype operator() (datatype x) const;
					};
				}
			}
		}
	}
}
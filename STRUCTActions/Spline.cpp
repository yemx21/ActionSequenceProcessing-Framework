#include "Spline.h"
#include <cstdio>
#include <cassert>
#include <vector>
#include <algorithm>

using namespace rsdn;
using namespace rsdn::learning;
using namespace rsdn::learning::timeseries;
using namespace rsdn::learning::timeseries::operators;
using namespace rsdn::learning::timeseries::operators::details;

band_matrix::band_matrix(int dim, int n_u, int n_l)
{
	resize(dim, n_u, n_l);
}
void band_matrix::resize(int dim, int n_u, int n_l)
{
	assert(dim>0);
	assert(n_u >= 0);
	assert(n_l >= 0);
	m_upper.resize(n_u + 1);
	m_lower.resize(n_l + 1);
	for (size_t i = 0; i<m_upper.size(); i++) {
		m_upper[i].resize(dim);
	}
	for (size_t i = 0; i<m_lower.size(); i++) {
		m_lower[i].resize(dim);
	}
}
int band_matrix::dim() const
{
	if (m_upper.size()>0) {
		return m_upper[0].size();
	}
	else {
		return 0;
	}
}


datatype & band_matrix::operator () (int i, int j)
{
	int k = j - i;   
	assert((i >= 0) && (i<dim()) && (j >= 0) && (j<dim()));
	assert((-num_lower() <= k) && (k <= num_upper()));
	if (k >= 0)   return m_upper[k][i];
	else	    return m_lower[-k][i];
}
datatype band_matrix::operator () (int i, int j) const
{
	int k = j - i;  
	assert((i >= 0) && (i<dim()) && (j >= 0) && (j<dim()));
	assert((-num_lower() <= k) && (k <= num_upper()));
	if (k >= 0)   return m_upper[k][i];
	else	    return m_lower[-k][i];
}
datatype band_matrix::saved_diag(int i) const
{
	assert((i >= 0) && (i<dim()));
	return m_lower[0][i];
}
datatype & band_matrix::saved_diag(int i)
{
	assert((i >= 0) && (i<dim()));
	return m_lower[0][i];
}

void band_matrix::lu_decompose()
{
	int  i_max, j_max;
	int  j_min;
	datatype x;

	for (int i = 0; i<this->dim(); i++) {
		assert(this->operator()(i, i) != 0.0);
		this->saved_diag(i) = 1.0 / this->operator()(i, i);
		j_min = std::max(0, i - this->num_lower());
		j_max = std::min(this->dim() - 1, i + this->num_upper());
		for (int j = j_min; j <= j_max; j++) {
			this->operator()(i, j) *= this->saved_diag(i);
		}
		this->operator()(i, i) = 1.0; 
	}

	for (int k = 0; k<this->dim(); k++) {
		i_max = std::min(this->dim() - 1, k + this->num_lower());
		for (int i = k + 1; i <= i_max; i++) {
			assert(this->operator()(k, k) != 0.0);
			x = -this->operator()(i, k) / this->operator()(k, k);
			this->operator()(i, k) = -x;                        
			j_max = std::min(this->dim() - 1, k + this->num_upper());
			for (int j = k + 1; j <= j_max; j++) {
				this->operator()(i, j) = this->operator()(i, j) + x*this->operator()(k, j);
			}
		}
	}
}

std::vector<datatype> band_matrix::l_solve(const std::vector<datatype>& b) const
{
	assert(this->dim() == (int)b.size());
	std::vector<datatype> x(this->dim());
	int j_start;
	datatype sum;
	for (int i = 0; i<this->dim(); i++) {
		sum = 0;
		j_start = std::max(0, i - this->num_lower());
		for (int j = j_start; j<i; j++) sum += this->operator()(i, j)*x[j];
		x[i] = (b[i] * this->saved_diag(i)) - sum;
	}
	return x;
}

std::vector<datatype> band_matrix::r_solve(const std::vector<datatype>& b) const
{
	assert(this->dim() == (int)b.size());
	std::vector<datatype> x(this->dim());
	int j_stop;
	datatype sum;
	for (int i = this->dim() - 1; i >= 0; i--) {
		sum = 0;
		j_stop = std::min(this->dim() - 1, i + this->num_upper());
		for (int j = i + 1; j <= j_stop; j++) sum += this->operator()(i, j)*x[j];
		x[i] = (b[i] - sum) / this->operator()(i, i);
	}
	return x;
}

std::vector<datatype> band_matrix::lu_solve(const std::vector<datatype>& b,
	bool is_lu_decomposed)
{
	assert(this->dim() == (int)b.size());
	std::vector<datatype>  x, y;
	if (is_lu_decomposed == false) {
		this->lu_decompose();
	}
	y = this->l_solve(b);
	x = this->r_solve(y);
	return x;
}

void Spline::set_boundary(Spline::bd_type left, datatype left_value,
	Spline::bd_type right, datatype right_value,
	bool force_linear_extrapolation)
{
	assert(m_x.size() == 0);
	m_left = left;
	m_right = right;
	m_left_value = left_value;
	m_right_value = right_value;
	m_force_linear_extrapolation = force_linear_extrapolation;
}


void Spline::set_points(const std::vector<datatype>& x,
	const std::vector<datatype>& y, bool cubic_spline)
{
	assert(x.size() == y.size());
	assert(x.size()>2);
	m_x = x;
	m_y = y;
	int   n = x.size();
	for (int i = 0; i<n - 1; i++) {
		assert(m_x[i]<m_x[i + 1]);
	}

	if (cubic_spline == true) {
								
		band_matrix A(n, 1, 1);
		std::vector<datatype>  rhs(n);
		for (int i = 1; i<n - 1; i++) {
			A(i, i - 1) = 1.0 / 3.0*(x[i] - x[i - 1]);
			A(i, i) = 2.0 / 3.0*(x[i + 1] - x[i - 1]);
			A(i, i + 1) = 1.0 / 3.0*(x[i + 1] - x[i]);
			rhs[i] = (y[i + 1] - y[i]) / (x[i + 1] - x[i]) - (y[i] - y[i - 1]) / (x[i] - x[i - 1]);
		}

		if (m_left == Spline::second_deriv) {
			
			A(0, 0) = 2.0;
			A(0, 1) = 0.0;
			rhs[0] = m_left_value;
		}
		else if (m_left == Spline::first_deriv) {
			A(0, 0) = 2.0*(x[1] - x[0]);
			A(0, 1) = 1.0*(x[1] - x[0]);
			rhs[0] = 3.0*((y[1] - y[0]) / (x[1] - x[0]) - m_left_value);
		}
		else {
			assert(false);
		}
		if (m_right == Spline::second_deriv) {
			A(n - 1, n - 1) = 2.0;
			A(n - 1, n - 2) = 0.0;
			rhs[n - 1] = m_right_value;
		}
		else if (m_right == Spline::first_deriv) {
			A(n - 1, n - 1) = 2.0*(x[n - 1] - x[n - 2]);
			A(n - 1, n - 2) = 1.0*(x[n - 1] - x[n - 2]);
			rhs[n - 1] = 3.0*(m_right_value - (y[n - 1] - y[n - 2]) / (x[n - 1] - x[n - 2]));
		}
		else {
			assert(false);
		}

		m_b = A.lu_solve(rhs);

		m_a.resize(n);
		m_c.resize(n);
		for (int i = 0; i<n - 1; i++) {
			m_a[i] = 1.0 / 3.0*(m_b[i + 1] - m_b[i]) / (x[i + 1] - x[i]);
			m_c[i] = (y[i + 1] - y[i]) / (x[i + 1] - x[i])
				- 1.0 / 3.0*(2.0*m_b[i] + m_b[i + 1])*(x[i + 1] - x[i]);
		}
	}
	else { 
		m_a.resize(n);
		m_b.resize(n);
		m_c.resize(n);
		for (int i = 0; i<n - 1; i++) {
			m_a[i] = 0.0;
			m_b[i] = 0.0;
			m_c[i] = (m_y[i + 1] - m_y[i]) / (m_x[i + 1] - m_x[i]);
		}
	}

	m_b0 = (m_force_linear_extrapolation == false) ? m_b[0] : 0.0;
	m_c0 = m_c[0];

	datatype h = x[n - 1] - x[n - 2];
	m_a[n - 1] = 0.0;
	m_c[n - 1] = 3.0*m_a[n - 2] * h*h + 2.0*m_b[n - 2] * h + m_c[n - 2]; 
	if (m_force_linear_extrapolation == true)
		m_b[n - 1] = 0.0;
}

bool Spline::set_points1(const datatype* x,
	const datatype* y, size_t n, bool filternan, bool cubic_spline)
{
	if (!filternan)
	{
		m_x.resize(n);
		m_y.resize(n);
		memcpy(m_x.data(), x, sizeof(datatype)*n);
		memcpy(m_y.data(), y, sizeof(datatype)*n);
	}
	else
	{
		for (size_t i = 0; i < n; i++)
		{
			if (!isnan(y[i]))
			{
				m_x.push_back(x[i]);
				m_y.push_back(y[i]);
			}
		}
		if (m_x.size() < 2) return false;
	}

	n = m_x.size();

	if (cubic_spline == true) {

		band_matrix A(n, 1, 1);
		std::vector<datatype>  rhs(n);
		for (int i = 1; i<n - 1; i++) {
			A(i, i - 1) = 1.0 / 3.0*(m_x[i] - m_x[i - 1]);
			A(i, i) = 2.0 / 3.0*(m_x[i + 1] - m_x[i - 1]);
			A(i, i + 1) = 1.0 / 3.0*(m_x[i + 1] - m_x[i]);
			rhs[i] = (m_y[i + 1] - m_y[i]) / (m_x[i + 1] - m_x[i]) - (m_y[i] - m_y[i - 1]) / (m_x[i] - m_x[i - 1]);
		}

		if (m_left == Spline::second_deriv) {

			A(0, 0) = 2.0;
			A(0, 1) = 0.0;
			rhs[0] = m_left_value;
		}
		else if (m_left == Spline::first_deriv) {
			A(0, 0) = 2.0*(m_x[1] - m_x[0]);
			A(0, 1) = 1.0*(m_x[1] - m_x[0]);
			rhs[0] = 3.0*((m_y[1] - m_y[0]) / (m_x[1] - m_x[0]) - m_left_value);
		}
		else {
			return false;
		}
		if (m_right == Spline::second_deriv) {
			A(n - 1, n - 1) = 2.0;
			A(n - 1, n - 2) = 0.0;
			rhs[n - 1] = m_right_value;
		}
		else if (m_right == Spline::first_deriv) {
			A(n - 1, n - 1) = 2.0*(m_x[n - 1] - m_x[n - 2]);
			A(n - 1, n - 2) = 1.0*(m_x[n - 1] - m_x[n - 2]);
			rhs[n - 1] = 3.0*(m_right_value - (m_y[n - 1] - m_y[n - 2]) / (m_x[n - 1] - m_x[n - 2]));
		}
		else {
			return false;
		}

		m_b = A.lu_solve(rhs);

		m_a.resize(n);
		m_c.resize(n);
		for (int i = 0; i<n - 1; i++) {
			m_a[i] = 1.0 / 3.0*(m_b[i + 1] - m_b[i]) / (m_x[i + 1] - m_x[i]);
			m_c[i] = (m_y[i + 1] - m_y[i]) / (m_x[i + 1] - m_x[i])
				- 1.0 / 3.0*(2.0*m_b[i] + m_b[i + 1])*(m_x[i + 1] - m_x[i]);
		}
	}
	else {
		m_a.resize(n);
		m_b.resize(n);
		m_c.resize(n);
		for (int i = 0; i<n - 1; i++) {
			m_a[i] = 0.0;
			m_b[i] = 0.0;
			m_c[i] = (m_y[i + 1] - m_y[i]) / (m_x[i + 1] - m_x[i]);
		}
	}

	m_b0 = (m_force_linear_extrapolation == false) ? m_b[0] : 0.0;
	m_c0 = m_c[0];

	datatype h = m_x[n - 1] - m_x[n - 2];
	m_a[n - 1] = 0.0;
	m_c[n - 1] = 3.0*m_a[n - 2] * h*h + 2.0*m_b[n - 2] * h + m_c[n - 2];
	if (m_force_linear_extrapolation == true)
		m_b[n - 1] = 0.0;

	return true;
}

void Spline::interp_points(const datatype* x,
	datatype* y, size_t len)
{
	for (int i = 0; i < len; i++)
	{
		y[i] = operator()(x[i]);
	}
}

datatype Spline::operator() (datatype x) const
{
	size_t n = m_x.size();

	std::vector<datatype>::const_iterator it;
	it = std::lower_bound(m_x.begin(), m_x.end(), x);
	int idx = std::max(int(it - m_x.begin()) - 1, 0);

	datatype h = x - m_x[idx];
	datatype interpol;
	if (x<m_x[0]) {
		interpol = (m_b0*h + m_c0)*h + m_y[0];
	}
	else if (x>m_x[n - 1]) {
		interpol = (m_b[n - 1] * h + m_c[n - 1])*h + m_y[n - 1];
	}
	else {
		interpol = ((m_a[idx] * h + m_b[idx])*h + m_c[idx])*h + m_y[idx];
	}
	return interpol;
}

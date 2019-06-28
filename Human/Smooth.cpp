#include "Interpolator.h"
#include <algorithm>
using namespace rsdn;
using namespace std;
using namespace rsdn::learning::timeseries::details;

void lowess(const vector<double> &x, const vector<double> &y, double f, long nsteps, double delta, vector<double> &ys, vector<double> &rw, vector<double> &res);

void lowess(const vector<double> &x, const vector<double> &y, double f, long nsteps, vector<double> &ys);

void lowest(const vector<double> &x, const vector<double> &y, double xs, double &ys, long nleft, long nright, vector<double> &w, bool userw, vector<double> &rw, bool &ok);

void lowess(const vector<double> &x, const vector<double> &y, double f, long nsteps, vector<double> &ys)
{
	vector<double> rw, res;
	lowess(x, y, f, nsteps, 0., ys, rw, res);
}

void lowess(const vector<double> &x, const vector<double> &y, double f, long nsteps, double delta, vector<double> &ys, vector<double> &rw, vector<double>&res) { //{{{
	long n = (long)x.size();
	bool ok = false;
	long nleft, nright, i, j, iter, last, m1, m2, ns;
	double cut, cmad, r, d1, d2, c1, c9, alpha, denom;
	if ((n == 0) || ((long)y.size() != n)) return;
	ys.resize(n);
	rw.resize(n);
	res.resize(n);
	if (n == 1) {
		ys[0] = y[0];
		return;
	}
	// ns - at least 2, at most n
	ns = max(min((long)(f*n), n), (long)2);
	for (iter = 0; iter<nsteps + 1; iter++) {
		// robustnes iterations
		nleft = 0;
		nright = ns - 1;
		// index of last estimated point
		last = -1;
		// index of current point
		i = 0;
		do {
			while (nright<n - 1) {
				// move <nleft,nright> right, while radius decreases
				d1 = x[i] - x[nleft];
				d2 = x[nright + 1] - x[i];
				if (d1 <= d2)break;
				nleft++;
				nright++;
			}
			// fit value at x[i]
			lowest(x, y, x[i], ys[i], nleft, nright, res, iter>0, rw, ok);
			if (!ok) ys[i] = y[i];
			if (last<i - 1) {
				// interpolate skipped points
				if (last<0) {
					//warning("Lowess: out of range.\n");
				}
				denom = x[i] - x[last];
				for (j = last + 1; j<i; j++) {
					alpha = (x[j] - x[last]) / denom;
					ys[j] = alpha * ys[i] + (1.0 - alpha)*ys[last];
				}
			}
			last = i;
			cut = x[last] + delta;
			for (i = last + 1; i<n; i++) {
				if (x[i]>cut)break;
				if (x[i] == x[last]) {
					ys[i] = ys[last];
					last = i;
				}
			}
			i = max(last + 1, i - 1);
		} while (last<n - 1);
		for (i = 0; i<n; i++)
			res[i] = y[i] - ys[i];
		if (iter == nsteps)break;
		for (i = 0; i<n; i++)
			rw[i] = abs(res[i]);
		sort(rw.begin(), rw.end());
		m1 = n / 2 + 1;
		m2 = n - m1;
		m1--;
		cmad = 3.0 *(rw[m1] + rw[m2]);
		c9 = .999*cmad;
		c1 = .001*cmad;
		for (i = 0; i<n; i++) {
			r = abs(res[i]);
			if (r <= c1) rw[i] = 1;
			else if (r>c9) rw[i] = 0;
			else rw[i] = (1.0 - (r / cmad)*(r / cmad))*(1.0 - (r / cmad)*(r / cmad));
		}
	}
}//}}}

void lowest(const vector<double> &x, const vector<double> &y, double xs, double &ys, long nleft, long nright, vector<double> &w, bool userw, vector<double> &rw, bool &ok) {//{{{
	long n = (long)x.size();
	long nrt, j;
	double a, b, c, h, r, h1, h9, range;
	range = x[n - 1] - x[0];
	h = max(xs - x[nleft], x[nright] - xs);
	h9 = 0.999*h;
	h1 = 0.001*h;
	// sum of weights
	a = 0;
	for (j = nleft; j<n; j++) {
		// compute weights (pick up all ties on right)
		w[j] = 0.;
		r = abs(x[j] - xs);
		if (r <= h9) {
			// small enough for non-zero weight
			if (r>h1) w[j] = (1.0 - (r / h)*(r / h)*(r / h))*(1.0 - (r / h)*(r / h)*(r / h))*(1.0 - (r / h)*(r / h)*(r / h));
			else w[j] = 1.;
			if (userw) w[j] *= rw[j];
			a += w[j];
		}
		else if (x[j]>xs) break; // get out at first zero wt on right
	}
	nrt = j - 1;
	// rightmost pt (may be greater than nright because of ties)
	if (a <= 0.) ok = false;
	else {
		// weighted least squares
		ok = true;
		// normalize weights
		for (j = nleft; j <= nrt; j++)
			w[j] /= a;
		if (h>0.) {
			// use linear fit
			a = 0.;
			for (j = nleft; j <= nrt; j++)
				a += w[j] * x[j]; // weighted centre of values
			b = xs - a;
			c = 0;
			for (j = nleft; j <= nrt; j++)
				c += w[j] * (x[j] - a)*(x[j] - a);
			if (sqrt(c)>0.001*range) {
				// points are spread enough to compute slope
				b /= c;
				for (j = nleft; j <= nrt; j++)
					w[j] *= (1.0 + b*(x[j] - a));
			}
		}
		ys = 0;
		for (j = nleft; j <= nrt; j++)
			ys += w[j] * y[j];
	}
}//}}}

bool Interpolator::Smooth(Trajectory* target)
{
	if (!target) return false;
	int count = target->data.size();
	double fr_ti = *target->first_timeStamp;
	vector<double> t;
	t.reserve(count);
	vector<double> x;
	vector<double> y;
	vector<double> z;
	x.reserve(count);
	y.reserve(count);
	z.reserve(count);
	for (int i = 0; i < count; i++)
	{
		double ti = target->data[i].time;
		if (ti >= fr_ti)
		{
			t.push_back(ti);
			x.push_back(target->data[i].point.x());
			y.push_back(target->data[i].point.y());
			z.push_back(target->data[i].point.z());
		}
	}

	vector<double> rx;
	vector<double> ry;
	vector<double> rz;

	double dur = fabs(t.back() - t.front());
	double span = 166.677 / dur;
	lowess(t, x, span, 0, rx);
	lowess(t, y, span, 0, ry);
	lowess(t, z, span, 0, rz);

	int n = 0;
	for (int i = 0; i < count; i++)
	{
		double ti = target->data[i].time;
		if (ti >= fr_ti)
		{
			target->data[i].point.x() = rx[n];
			target->data[i].point.y() = ry[n];
			target->data[i].point.z() = rz[n];
			n++;
		}
	}
	return true;
}
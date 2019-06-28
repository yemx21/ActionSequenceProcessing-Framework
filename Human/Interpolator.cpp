#include "Interpolator.h"

using namespace rsdn;
using namespace rsdn::learning::timeseries::details;

#pragma region Trajectory

Trajectory::Point3Cache::Point3Cache() : time(-1.0), phase(-1), need(true), point(NAN, NAN, NAN)
{

}

Trajectory::Point3Cache::Point3Cache(datatype t, int ph) : time(t), phase(ph), need(true), point(NAN, NAN, NAN)
{

}

Trajectory::Point3Cache::Point3Cache(datatype t, int ph, const Point3& pt) : time(t), phase(ph), point(pt), need(false)
{

}

Trajectory::Point3Cache::Point3Cache(const Trajectory::Point3Cache& b)
{
	time = b.time;
	phase = b.phase;
	point = b.point;
	need = b.need;
}

bool Trajectory::Point3Cache::operator<(const Trajectory::Point3Cache& b)const
{
	return time < b.time;
}

Trajectory::Trajectory() : keyframe(0)
{

}

void Trajectory::Push(datatype timeStamp, int ph, const Point3& pt)
{
	if (!first_timeStamp) first_timeStamp = timeStamp;
	data.emplace_back(timeStamp, ph, pt);
}

void Trajectory::Push(datatype timeStamp, int ph)
{
	data.emplace_back(timeStamp, ph);
}

void Trajectory::SetFirstKeyFrame(size_t index)
{
	keyframe = index;
}

unsigned int Trajectory::GetCount() const
{
	return data.size();
}

bool Trajectory::GetPoint(Point3& pt, datatype& timeidx, int& ph, unsigned int index)
{
	if (index < data.size())
	{
		if (data[index].need) return false;
		pt = data[index].point;
		timeidx = data[index].time;
		ph = data[index].phase;
		return true;
	}
	return false;
}

void Trajectory::Clear()
{
	first_timeStamp.Reset();
	data.clear();
}
#pragma endregion

#pragma region FUNCS

const datatype Interpolator_Const_61 = 1.0 / 6.0;
//const datatype Interpolator_Const_241 = 1.0 / 24.0;
//const datatype Interpolator_Const_1201 = 1.0 / 120.0;

#define CAL_ACC1(RES,TIME) RES.start + RES.velocity*TIME + RES.acceleration* 0.5 *TIME *TIME 
#define CAL_ACC2(RES,TIME) RES.start + RES.velocity*TIME + RES.acceleration* 0.5 *TIME *TIME + RES.acceleration_derivative * Interpolator_Const_61 *TIME *TIME *TIME
//#define CAL_ACC3(RES,TIME) RES.start + RES.velocity*TIME + RES.acceleration* 0.5 *TIME *TIME + RES.acceleration_derivative * Interpolator_Const_61 *TIME *TIME *TIME + RES.acceleration_derivative2 * Interpolator_Const_241 *TIME *TIME *TIME*TIME
//#define CAL_ACC4(RES,TIME) RES.start + RES.velocity*TIME + RES.acceleration* 0.5 *TIME *TIME + RES.acceleration_derivative * Interpolator_Const_61 *TIME *TIME *TIME + RES.acceleration_derivative2 * Interpolator_Const_241 *TIME *TIME *TIME*TIME+ RES.acceleration_derivative3 * Interpolator_Const_1201 *TIME *TIME *TIME*TIME*TIME

template<size_t N>
struct Counter_impl
{
	size_t data[N];
	size_t count;
	Counter_impl() :count(0u)
	{
	}

	void push(size_t idx)
	{
		if (count < N)
		{
			data[count] = idx;
			count++;
		}
	}

	void push_front(size_t idx)
	{
		if (count < N)
		{
			data[N - count - 1] = idx;
			count++;
		}
	}

	void clear()
	{
		count = 0;
	}

	bool full() const
	{
		return count >= N;
	}

	bool full(size_t n) const
	{
		return count >= n;
	}
};

typedef Counter_impl<4> Counter;

struct MotionKnot
{
	datatype time;
	Point3 pos;
	Vector3 velocity;
	Vector3 acceleration;
	Vector3 acceleration_derivative;
	MotionKnot(const Trajectory::Point3Cache& c) :time(c.time), pos(c.point)
	{

	}

	MotionKnot(datatype t, const Point3& p) :time(t), pos(p)
	{

	}

	bool operator<(const MotionKnot& b)const
	{
		return time < b.time;
	}
};

struct MotionGap : public std::vector<size_t>
{
	datatype start;
	datatype end;
	size_t startidx;
	size_t endidx;
	MotionGap(size_t idx, datatype t)
	{
		startidx = idx;
		endidx = idx;
		start = t;
		end = t;
		push_back(idx);
	}

	void add(size_t idx, datatype t)
	{
		end = t;
		endidx = idx;
		push_back(idx);
	}

};

struct MotionGapCollection
{
	std::vector<MotionGap*> gaps;
	MotionGap* cur;
	size_t next;

	MotionGapCollection() : cur(nullptr), next(-1)
	{

	}

	~MotionGapCollection()
	{
		for (MotionGap* mg : gaps)
		{
			if (mg) { delete mg; mg = nullptr; }
		}
		gaps.clear();
	}

	void create(size_t idx, datatype time)
	{
		cur = new MotionGap(idx, time);
		gaps.push_back(cur);
	}

	void add(size_t idx, datatype time)
	{
		if (cur) cur->add(idx, time);
	}

	bool push(size_t idx, Trajectory*  target)
	{
		const Trajectory::Point3Cache& cache = target->data[idx];
		if (target->data[idx].need)
		{
			if (idx != next)
			{
				create(idx, cache.time);
			}
			else
			{
				add(idx, cache.time);
			}
			next = idx + 1;
			return true;
		}
		return false;
	}

	bool push(size_t idx, datatype time)
	{
		if (idx != next)
		{
			create(idx, time);
		}
		else
		{
			add(idx, time);
		}
		next = idx + 1;
		return true;
	}

	bool empty() const
	{
		return gaps.empty();
	}

	MotionGap* back()
	{
		return gaps.back();
	}

	MotionGap* at(size_t pos)
	{
		if (pos< gaps.size()) return gaps.at(pos);
		return nullptr;
	}

	std::vector<MotionGap*>::iterator begin()
	{
		return gaps.begin();
	}

	std::vector<MotionGap*>::const_iterator begin() const
	{
		return gaps.begin();
	}

	std::vector<MotionGap*>::iterator end()
	{
		return gaps.end();
	}

	std::vector<MotionGap*>::const_iterator end() const
	{
		return gaps.end();
	}

};

struct MotionKnotCollection : public std::vector < MotionKnot >
{

#pragma region KnotMeasurement
	template<size_t PREV, size_t NEXT>
	struct KnotMeasurement;

	template<>
	struct KnotMeasurement < 0, 2 >
	{
		Vector3 velocity;
		Point3 start;
		datatype time;
		KnotMeasurement(const MotionKnot& pos1, const MotionKnot& pos2)
		{
			start = pos1.pos;
			time = pos1.time;
			velocity = (pos2.pos - pos1.pos) / (pos2.time - pos1.time);
		}
	};

	template<>
	struct KnotMeasurement < 2, 0 >
	{
		Vector3 velocity;
		Point3 start;
		datatype time;
		KnotMeasurement(const MotionKnot& pos1, const MotionKnot& pos2)
		{
			start = pos2.pos;
			time = pos2.time;
			velocity = (pos2.pos - pos1.pos) / (pos2.time - pos1.time);
		}
	};

	template<>
	struct KnotMeasurement < 1, 1 >
	{
		Vector3 velocity;
		Point3 start;
		datatype time;
		KnotMeasurement(const MotionKnot& pos1, const MotionKnot& pos2)
		{
			start = pos1.pos;
			time = pos1.time;
			velocity = (pos2.pos - pos1.pos) / (pos2.time - pos1.time);
		}
	};

	template<>
	struct KnotMeasurement < 1, 2 >
	{
		Vector3 velocityfactor;
		Point3 start;
		datatype time;
		KnotMeasurement(const MotionKnot& pos1, const MotionKnot& pos2, const MotionKnot& pos3)
		{
			datatype dur12 = pos2.time - pos1.time;
			Vector3 velocity2 = (pos2.pos - pos1.pos) / dur12;
			Vector3 acceleration_12v3 = (pos3.velocity - velocity2) / (pos3.time - pos2.time);
			Vector3 velocity1 = velocity2 - acceleration_12v3*dur12;

			start = pos1.pos;
			time = pos1.time;
			velocityfactor = (velocity1 + velocity2)*0.5;
		}
	};

	template<>
	struct KnotMeasurement < 2, 1 >
	{
		Vector3 velocity;
		Vector3 acceleration;
		Vector3 acceleration_derivative;
		Point3 start;
		datatype time;
		KnotMeasurement(const MotionKnot& pos1, const MotionKnot& pos2, const MotionKnot& pos3)
		{
			start = pos2.pos;
			time = pos2.time;

			velocity = pos2.velocity;

			datatype dur23 = pos3.time - pos2.time;
			Vector3 velocity3 = (pos3.pos - pos2.pos) / dur23;
			acceleration = (velocity3 - pos2.velocity) / dur23;
			acceleration_derivative = (acceleration - pos2.acceleration) / dur23;
		}
	};

	template<>
	struct KnotMeasurement < 2, 2 >
	{
		Vector3 velocity;
		Vector3 acceleration;
		Vector3 acceleration_derivative;
		Point3 start;
		datatype time;
		KnotMeasurement(const MotionKnot& pos1, const MotionKnot& pos2, const MotionKnot& pos3, const MotionKnot& pos4)
		{
			start = pos2.pos;
			time = pos2.time;
			datatype dur23 = pos3.time - pos2.time;
			Vector3 velocity3 = (pos3.pos - pos2.pos) / dur23;
			velocity = (velocity3 + pos2.velocity) / 2;

			acceleration = (pos4.velocity - velocity) / (pos4.time - pos2.time);

			acceleration_derivative = (pos3.pos - pos2.pos - velocity* dur23 - acceleration *(dur23*dur23*0.5)) / (dur23*dur23*dur23*Interpolator_Const_61);

		}
	};

#pragma endregion

	bool process(const Vector3& initialvelocity)
	{
		if (size() == 0) return false;
		size_t count = size();
		if (count > 1)
		{
			operator[](0).velocity = initialvelocity;

			for (size_t idx = 1; idx < count; idx++)
			{
				MotionKnot& cur = at(idx);
				MotionKnot& prev = at(idx - 1);
				cur.velocity = (cur.pos - prev.pos) / (cur.time - prev.time);
			}

			for (size_t idx = 1; idx < count; idx++)
			{
				MotionKnot& cur = at(idx);
				MotionKnot& prev = at(idx - 1);
				cur.acceleration = (cur.velocity - prev.velocity) / (cur.time - prev.time);
			}

			if (count > 2)
			{
				for (size_t idx = 2; idx < count; idx++)
				{
					MotionKnot& cur = at(idx);
					MotionKnot& prev = at(idx - 1);
					cur.acceleration_derivative = (cur.acceleration - prev.acceleration) / (cur.time - prev.time);
				}
			}
		}
		return true;
	}

#pragma region Measure
	template<size_t PREV, size_t NEXT>
	KnotMeasurement<PREV, NEXT> Measure(Counter& prev, Counter& next);

	template<>
	KnotMeasurement<0, 2> Measure(Counter& prev, Counter& next)
	{
		return KnotMeasurement < 0, 2 > {at(next.data[0]), at(next.data[1])};
	}

	template<>
	KnotMeasurement<1, 1> Measure(Counter& prev, Counter& next)
	{
		return KnotMeasurement < 1, 1 > {at(prev.data[3]), at(next.data[0])};
	}

	template<>
	KnotMeasurement<1, 2> Measure(Counter& prev, Counter& next)
	{
		return KnotMeasurement < 1, 2 > {at(prev.data[3]), at(next.data[0]), at(next.data[1])};
	}

	template<>
	KnotMeasurement<2, 0> Measure(Counter& prev, Counter& next)
	{
		return KnotMeasurement < 2, 0 > {at(prev.data[2]), at(prev.data[3])};
	}

	template<>
	KnotMeasurement<2, 1> Measure(Counter& prev, Counter& next)
	{
		return KnotMeasurement < 2, 1 > {at(prev.data[2]), at(prev.data[3]), at(next.data[0])};
	}

	template<>
	KnotMeasurement<2, 2> Measure(Counter& prev, Counter& next)
	{
		return KnotMeasurement < 2, 2 > {at(prev.data[2]), at(prev.data[3]), at(next.data[0]), at(next.data[1])};
	}

#pragma endregion
	void gapfilling(MotionGap* gap, Trajectory*  target)
	{
		//int limit = 2;
		int limit = 1;
		if (gap->size() > 1 && abs(gap->start - gap->end) > 120.0)  limit = 1;

		size_t sidx = gap->startidx;
		size_t count = size();
		if (sidx > count - 1) sidx = count - 1;
		Optional<int> ridx;
		Counter prev_knot;
		for (int i = (int)sidx; i >= 0; i--)
		{
			if (has_nan(operator[](i).pos)) continue;
			if (operator[](i).time < gap->start)
			{
				if (!ridx) ridx = i;
				prev_knot.push_front(i);
			}
			if (prev_knot.full(limit)) break;
		}
		Counter next_knot;
		if (ridx)
		{
			for (int i = (*ridx) + 1; i<count; i++)
			{
				if (has_nan(operator[](i).pos)) continue;
				if (operator[](i).time > gap->end)
				{
					next_knot.push(i);
				}
				if (next_knot.full(limit)) break;
			}
		}

		switch (prev_knot.count)
		{
#pragma region Zero
		case 0:
		{
			switch (next_knot.count)
			{
			case 2:
			{
				KnotMeasurement < 0, 2 > res02 = Measure<0, 2>(prev_knot, next_knot);
				for (size_t i = gap->startidx; i <= gap->endidx; i++)
				{
					Trajectory::Point3Cache& pt = target->data[i];
					pt.point = res02.start + res02.velocity* (pt.time - res02.time);
				}
			}
			break;
			}
		}
		break;
#pragma endregion
#pragma region One
		case 1:
		{
			switch (next_knot.count)
			{
			case 1:
			{
				KnotMeasurement < 1, 1 > res11 = Measure<1, 1>(prev_knot, next_knot);
				for (size_t i = gap->startidx; i <= gap->endidx; i++)
				{
					Trajectory::Point3Cache& pt = target->data[i];
					datatype cur = pt.time - res11.time;
					pt.point = res11.start + res11.velocity*cur;
				}
			}
			break;
			case 2:
			{
				KnotMeasurement < 1, 2> res12 = Measure<1, 2>(prev_knot, next_knot);
				for (size_t i = gap->startidx; i <= gap->endidx; i++)
				{
					Trajectory::Point3Cache& pt = target->data[i];
					datatype cur = pt.time - res12.time;
					pt.point = res12.start + res12.velocityfactor*cur;
				}
			}
			break;
			}
		}
		break;
#pragma endregion
#pragma region Two
		case 2:
		{
			switch (next_knot.count)
			{
			case 0:
			{
				KnotMeasurement < 2, 0 > res20 = Measure<2, 0>(prev_knot, next_knot);
				for (size_t i = gap->startidx; i <= gap->endidx; i++)
				{
					Trajectory::Point3Cache& pt = target->data[i];
					pt.point = res20.start + res20.velocity* (pt.time - res20.time);
				}
			}
			break;
			case 1:
			{
				KnotMeasurement < 2, 1 > res21 = Measure<2, 1>(prev_knot, next_knot);
				for (size_t i = gap->startidx; i <= gap->endidx; i++)
				{
					Trajectory::Point3Cache& pt = target->data[i];
					datatype cur = pt.time - res21.time;
					pt.point = CAL_ACC2(res21, cur);
				}
			}
			break;
			case 2:
			{
				KnotMeasurement < 2, 2> res22 = Measure<2, 2>(prev_knot, next_knot);
				for (size_t i = gap->startidx; i <= gap->endidx; i++)
				{
					Trajectory::Point3Cache& pt = target->data[i];
					datatype cur = pt.time - res22.time;
					pt.point = CAL_ACC2(res22, cur);
				}
			}
			break;
			}
		}
		break;
#pragma endregion
		}

	}
};

#pragma endregion

bool Interpolator::Process(Trajectory*  target)
{
	size_t count = target->data.size();
	MotionGapCollection gaps;
	MotionKnotCollection knots;
	knots.reserve(16);

	size_t sidx = target->keyframe;
	size_t eidx = count;
	for (sidx; sidx < count; sidx++)
	{
		if (target->data[sidx].time >= *target->first_timeStamp) break;
	}

	for (eidx = count - 1; eidx > sidx; eidx--)
	{
		if (!has_nan(target->data[eidx].point)) break;
	}

	if (sidx >= count) return false;

	for (sidx; sidx < eidx; sidx++)
	{
		if (!gaps.push(sidx, target)) knots.emplace_back(target->data[sidx]);
	}

	if (!knots.process(Vector3{})) return false;

	for (MotionGap* gap : gaps)
	{
		knots.gapfilling(gap, target);
	}
	return true;
}

_inline int Interpolator_gcd(int a, int b)
{
	int min = (a>b) ? b : a;
	int max = (a>b) ? a : b;
	if (!min)
		return max;
	if (!max)
		return min;
	if (!(min & 1) && !(max & 1))
		return Interpolator_gcd(min >> 1, max >> 1) << 1;
	if (!(min & 1))
		return Interpolator_gcd(min >> 1, max);
	if (!(max & 1))
		return Interpolator_gcd(min, max >> 1);
	return Interpolator_gcd(max - min, min);
}

_inline int Interpolator_lcm(int a, int b)
{
	return a*b / Interpolator_gcd(a, b);
}

bool Interpolator::Resample(Trajectory*  target, unsigned int sr_src, unsigned int sr_dest)
{
	if (!target) return false;
	if (target->data.size() < 2) return false;
	if (sr_src == sr_dest) return true;
	Trajectory tmpdata{};
	int lcm = Interpolator_lcm((int)sr_src, (int)sr_dest);
	int up_ratio = lcm / sr_src;
	Optional<size_t> ibidx;
	std::vector<datatype> ibtimes;
	{
		size_t idx = 0;
		size_t count = target->data.size();
		for (idx; idx < count; idx++)
		{
			ibtimes.push_back(target->data[idx].time);
			if (target->data[idx].time >= *target->first_timeStamp) break;
		}
		if (idx >= count - 1) return false;
		ibidx = idx;
	}

	if (up_ratio != 1)
	{
		int up_gap = up_ratio - 1;
		const std::vector<Trajectory::Point3Cache>& data = target->data;
		size_t count = target->data.size();
		size_t idx = 0;
		size_t counter = 0;
		size_t keyframe = 0;

		datatype lasttime = data[idx].time;
		tmpdata.Push(data[idx].time, data[idx].phase, data[idx].point);
		idx++;
		size_t j = 1;
		for (idx; idx< count; idx++)
		{
			datatype dur = data[idx].time - lasttime;
			for (j = 1; j <= up_gap; j++)
			{
				tmpdata.Push(lasttime + j * dur / up_ratio, data[idx].phase);
				counter++;
			}
			if (idx == *ibidx)
				keyframe = counter;
			tmpdata.Push(data[idx].time, data[idx].phase, data[idx].point);
			counter++;
			lasttime = data[idx].time;
		}
		tmpdata.SetFirstKeyFrame(keyframe);
		if (!Process(&tmpdata)) return false;
	}
	else
	{
		tmpdata.data = std::move(target->data);
		tmpdata.data.erase(tmpdata.data.begin(), tmpdata.data.begin() + *ibidx - 1);
	}
	int down_ratio = lcm / sr_dest;
	if (down_ratio != 1)
	{
		int down_cut = down_ratio - 1;
		const std::vector<Trajectory::Point3Cache>& tdata = tmpdata.data;
		std::vector<Trajectory::Point3Cache>& data = target->data;
		data.clear();
		data.push_back(tdata[0]);
		int cut_counter = down_cut;
		for (size_t i = 1; i < tdata.size(); i++)
		{
			if (cut_counter > 0)
			{
				cut_counter--;
			}
			else
			{
				data.push_back(tdata[i]);
				cut_counter = down_cut;
			}
		}
	}
	else
	{
		target->data = std::move(tmpdata.data);
	}

	for (Trajectory::Point3Cache& pc : target->data)
	{
		pc.need = false;
	}

	return true;
}
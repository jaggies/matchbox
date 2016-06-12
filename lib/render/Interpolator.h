/*
 * interpolator.h
 *
 *  Created on: Jan 20, 2012
 *      Author: jmiller
 */

#ifndef INTERPOLATOR_H_
#define INTERPOLATOR_H_

template <class T, int fraction, int shift>
class Interpolator {
	public:
		Interpolator() { }
		Interpolator(const T from, const T& to, int steps, int offset=0) {
			start(from, to, steps, offset); }
		~Interpolator() { }
		void start(const T& from);
		void start(const T& from, const T& to, int steps, int offset=0);
		void advance(int n);
		T eval(int n) const;
		operator T() const;
		T nextValue();
	private:
		T _value;
		T _increment;
};

template <class T, int fraction, int shift>
inline
void Interpolator<T, fraction, shift>::start(const T& from) {
	_value = from << (fraction - shift);
}

template <class T, int fraction, int shift>
inline
void Interpolator<T, fraction, shift>::start(const T& from, const T& to, int steps, int offset)
{
	_increment = steps > 1 ? (((to - from) << (fraction - shift)) / (steps - 1)) : 0;
	const T rnd = 1 << (fraction - shift - 1); // pre-round fraction by adding half of lsb
	_value = offset * _increment + (from << (fraction - shift)) + rnd;
}

template <class T, int fraction, int shift>
inline
void Interpolator<T, fraction, shift>::advance(int n)
{
	_value += n*_increment;
}

template <class T, int fraction, int shift>
inline
T Interpolator<T, fraction, shift>::eval(int n) const {
	return _value + _increment * n;
}

template <class T, int fraction, int shift>
inline
Interpolator<T, fraction, shift>::operator T() const {
	return _value >> fraction;
}

template <class T, int fraction, int shift>
inline
T Interpolator<T, fraction, shift>::nextValue() {
	T tmp = _value; _value += _increment; return tmp >> fraction;
}

#endif /* INTERPOLATOR_H_ */

#ifndef _FIFO_H
#define _FIFO_H

#include <stdint.h>

template<class T, class S, S N>
class Fifo {
	public:
		Fifo() : _head(0), _tail(0) { }

		inline bool add(const T& data) {
			S newhead = (_head + 1);
			if (newhead == N)
				newhead = 0;
			if (newhead != _tail) {
				_data[_head] = data;
				_head = newhead;
				return true;
			}
			return false;
		}

		inline bool remove(T* data) {
			if (_tail != _head) {
				*data = _data[_tail];
				_tail = (_tail + 1);
				if (_tail == N)
					_tail = 0;
				return true;
			}
			return false;
		}

		inline bool isFull() const {
			S newhead = (_head + 1);
			if (newhead == N)
				newhead = 0;
			return newhead == _tail;
		}

		// Peek n items back in history
		inline const T& peek(S n) {
		    S idx = (_head + N - n) % N;
		    return _data[idx];
		}

		inline bool isEmpty() const { return _head == _tail; }

		inline S count() const { return _head >= _tail ? (_head - _tail) : (N - _tail + _head); }

		// Reset to empty state
		void clear() { _head = _tail = 0; }

		S size() const { return N; }
	private:
		T _data[N];
		volatile S _head;
		volatile S _tail;
};

#endif // _FIFO_H

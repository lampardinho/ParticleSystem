#include <vector>
#include <mutex>
#include <memory>

template <class T>
class circular_buffer {
public:
//	circular_buffer(int size) :
////		buf_(std::vector<T>(size)),
//		size_(size)
//	{
//		//empty constructor
//	}

	void reserve(int size)
	{
		buf_ = new T[size];

		size_ = size;
	}

	T& put();
	void removeLast(void);

	void reset(void)
	{
//		std::lock_guard<std::mutex> lock(mutex_);
		head_ = tail_;
	}

	

//	const T& operator[] (const int index) const // for const objects: can only be used for access
//	{
//		return buf_[(tail_ + index) % size_];
//	}

	bool empty(void);
	bool full(void);
	int size(void);
	T& operator[] (const int index);

	~circular_buffer()
	{
		if (buf_)
			delete[] buf_;
	}

private:
//	std::mutex mutex_;
	T* buf_ = nullptr;
	int head_ = 0;
	int tail_ = 0;
	int size_ = 0;
	int count = 0;
};

template <class T>
int circular_buffer<T>::size(void)
{
	return count;
}

template <class T>
T& circular_buffer<T>::operator[] (const int index)
{
	return buf_[(tail_ + index) % size_];
}

template <class T>
T& circular_buffer<T>::put()
{
//	std::lock_guard<std::mutex> lock(mutex_);
	int oldHead = head_;
	head_ = (head_ + 1) % size_;

	if (head_ == tail_)
	{
		tail_ = (tail_ + 1) % size_;
		count--;
	}
	
	count++;

	return buf_[oldHead];
}

template <class T>
void circular_buffer<T>::removeLast(void)
{
//	std::lock_guard<std::mutex> lock(mutex_);

	tail_ = (tail_ + 1) % size_;
	count--;
}
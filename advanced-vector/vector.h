#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <algorithm>

template <typename T>
class RawMemory {
public:
	RawMemory() = default;
	RawMemory(const RawMemory&) = delete;
	RawMemory& operator=(const RawMemory&) = delete;

	RawMemory(RawMemory&& input) noexcept {
		if (this != &input) {
			std::swap(buffer_, input.buffer_);
			std::swap(capacity_, input.capacity_);
		}
	};
	RawMemory& operator=(RawMemory&& input) noexcept {
		if (this != &input) {
			std::swap(buffer_, input.buffer_);
			std::swap(capacity_, input.capacity_);
		}
		return *this;
	};

	explicit RawMemory(size_t capacity)
		: buffer_(Allocate(capacity))
		, capacity_(capacity) {
	}

	~RawMemory() {
		Deallocate(buffer_);
	}

	T* operator+(size_t offset) noexcept {
		// Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
		assert(offset <= capacity_);
		return buffer_ + offset;
	}

	const T* operator+(size_t offset) const noexcept {
		return const_cast<RawMemory&>(*this) + offset;
	}

	const T& operator[](size_t index) const noexcept {
		return const_cast<RawMemory&>(*this)[index];
	}

	T& operator[](size_t index) noexcept {
		assert(index < capacity_);
		return buffer_[index];
	}

	void Swap(RawMemory& other) noexcept {
		std::swap(buffer_, other.buffer_);
		std::swap(capacity_, other.capacity_);
	}

	const T* GetAddress() const noexcept {
		return buffer_;
	}

	T* GetAddress() noexcept {
		return buffer_;
	}

	size_t Capacity() const {
		return capacity_;
	}

private:
	// Выделяет сырую память под n элементов и возвращает указатель на неё
	static T* Allocate(size_t n) {
		return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
	}

	// Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
	static void Deallocate(T* buf) noexcept {
		operator delete(buf);
	}

	T* buffer_ = nullptr;
	size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
	Vector() = default;
	explicit Vector(size_t size)
		: data_(size)
		, capacity_(size)
		, size_(size)
	{
		std::uninitialized_value_construct_n(data_.GetAddress(), size);
	}
	Vector(const Vector& other)
		: data_(other.size_)
		, capacity_(other.size_)
		, size_(other.size_)
	{
		if (this != &other) {
			std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
		}
	}

	Vector& operator=(const Vector& other)
	{
		if (this != &other) {
			if (capacity_ < other.capacity_) {
				Vector copy(other);
				Swap(copy);
			}
			else {
				size_t i = 0;
				for (; i < other.size_ && i < size_; ++i) {
					//(data_ + i) -> ~T();
					data_[i] = other.data_[i];
				}

				if (size_ > other.size_) {
					std::destroy_n(data_.GetAddress() + i, size_ - i);
				}
				else {
					std::uninitialized_copy_n(data_.GetAddress() + i, other.size_ - i, data_.GetAddress() + i);
				}
				size_ = other.size_;
			}
		}
		return *this;
	}

	Vector(Vector&& input) noexcept
	{
		if (this != &input) {
			data_ = std::move(input.data_);
			capacity_ = input.capacity_;
			size_ = input.size_;

			input.size_ = 0;
			input.capacity_ = 0;
		}
	}

	Vector& operator=(Vector&& input) noexcept
	{
		if (this != &input) {
			data_ = std::move(input.data_);
			capacity_ = input.capacity_;
			size_ = input.size_;

			input.size_ = 0;
			input.capacity_ = 0;
		}
		return *this;
	}

	~Vector() {
		std::destroy_n(data_.GetAddress(), size_);
	}

	using iterator = T*;
	using const_iterator = const T*;

	iterator begin() noexcept {
		return data_.GetAddress();
	}
	iterator end() noexcept {
		return data_.GetAddress() + size_;
	}
	const_iterator begin() const noexcept {
		return data_.GetAddress();
	}
	const_iterator end() const noexcept {
		return data_.GetAddress() + size_;
	}
	const_iterator cbegin() const noexcept {
		return data_.GetAddress();
	}
	const_iterator cend() const noexcept {
		return data_.GetAddress() + size_;
	}

	template <typename... Args>
	iterator Emplace(const_iterator pos, Args&&... args) {
		if (pos == end()) {
			return &EmplaceBack(std::forward<Args>(args)...);
		}
		size_t pos_n = pos - begin();
		if (size_ + 1 > capacity_) {
			size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
			RawMemory<T> new_data(new_capacity);
			new(new_data + pos_n) T(std::forward<Args>(args)...);
			CopyMove(data_.GetAddress(), pos_n, new_data.GetAddress());
			CopyMove(data_ + pos_n, size_ - pos_n, new_data + pos_n + 1);

			std::destroy_n(data_.GetAddress(), size_);
			data_.Swap(new_data);
			capacity_ = new_capacity;
			++size_;
			return data_ + pos_n;
		}
		else {
				new(end()) T(std::move(data_[size_]));
				std::move_backward(data_ + pos_n - 1, end() - 1, end());
				(data_ + pos_n) -> ~T();
				new(data_ + pos_n) T(std::forward<Args>(args)...);
				++size_;
			
			return data_ + pos_n;
		}
	}
	iterator Erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>) {
		size_t pos_n = pos - begin();
		std::move(data_ + pos_n + 1, data_ + size_, data_ + pos_n);
		(data_ + size_ - 1) -> ~T();
		--size_;
		return data_ + pos_n;
	}
	iterator Insert(const_iterator pos, const T& value) {
		return Emplace(pos, value);
	}
	iterator Insert(const_iterator pos, T&& value) {
		return Emplace(pos, std::forward<T>(value));
	}

	size_t Size() const noexcept {
		return size_;
	}

	size_t Capacity() const noexcept {
		return capacity_;
	}

	const T& operator[](size_t index) const noexcept {
		return const_cast<Vector&>(*this)[index];
	}

	T& operator[](size_t index) noexcept {
		assert(index < size_);
		return data_[index];
	}

	void Swap(Vector& input) noexcept {
		data_.Swap(input.data_);
		std::swap(capacity_, input.capacity_);
		std::swap(size_, input.size_);
	}
	void Reserve(size_t new_capacity) {
		if (new_capacity <= capacity_) {
			return;
		}
		RawMemory<T> new_data(new_capacity);
		CopyMove(data_.GetAddress(), size_, new_data.GetAddress());

		std::destroy_n(data_.GetAddress(), size_);
		data_.Swap(new_data);
		capacity_ = new_capacity;
	}

	void Resize(size_t new_size) {
		if (new_size == size_) return;
		if (new_size < size_) {
			for (size_t i = new_size; i < size_; ++i) {
				(data_ + i) -> ~T();
			}
			size_ = new_size;
		}
		else {
			Reserve(new_size);
			std::uninitialized_value_construct_n(data_ + size_, new_size - size_);
			size_ = new_size;
		}
	};
	void PushBack(const T& value) {
		if (size_ == capacity_) {
			RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
			new(new_data + size_) T(value);
			CopyMove(data_.GetAddress(), size_, new_data.GetAddress());
			std::destroy_n(data_.GetAddress(), size_);
			data_.Swap(new_data);
			capacity_ = size_ == 0 ? 1 : size_ * 2;
			++size_;
		}
		else
		{
			new(data_ + size_) T(std::move(value));
			++size_;
		}
	};
	void PushBack(T&& value) {
		if (size_ == capacity_) {
			RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
			new(new_data + size_) T(std::move(value));
			CopyMove(data_.GetAddress(), size_, new_data.GetAddress());
			std::destroy_n(data_.GetAddress(), size_);
			data_.Swap(new_data);
			capacity_ = size_ == 0 ? 1 : size_ * 2;
			++size_;
		}
		else
		{
			new(data_ + size_) T(std::move(value));
			++size_;
		}
	};
	void PopBack() noexcept {
		(data_ + (size_ - 1)) -> ~T();
		--size_;
	};

	template <typename... Types>
	T& EmplaceBack(Types&&... params) {
		if (size_ == capacity_) {
			RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
			auto ref = new(new_data + size_) T(std::forward<Types>(params)...);
			CopyMove(data_.GetAddress(), size_, new_data.GetAddress());
			std::destroy_n(data_.GetAddress(), size_);
			data_.Swap(new_data);
			capacity_ = size_ == 0 ? 1 : size_ * 2;
			++size_;
			return *ref;
		}
		else
		{
			auto ref = new(data_ + size_) T(std::forward<Types>(params)...);
			++size_;
			return *ref;
		}
	}

private:

	void CopyMove(T* start, size_t size, T* dest) {
		if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
			std::uninitialized_move_n(start, size, dest);
		}
		else {
			std::uninitialized_copy_n(start, size, dest);
		}
	}

	RawMemory<T> data_;
	size_t capacity_ = 0;
	size_t size_ = 0;
};
#ifndef RIME_ANY_H_
#define RIME_ANY_H_

/*
    模仿 boost::any 实现一个any工具类 hywang
*/

#include <typeinfo>
#include <type_traits>
#include <utility>
#include <memory>
#include <stdexcept>
#include <iostream>

namespace rime
{
    class any
    {
    public:
        // 默认构造函数
        any() : ptr_(nullptr) {}

        // 构造函数模板，存储任意类型的值
        template <typename T>
        any(const T &value) : ptr_(new holder<typename std::decay<T>::type>(value)) {}

        // 移动构造函数
        any(any &&other) noexcept : ptr_(other.ptr_)
        {
            other.ptr_ = nullptr;
        }

        // 析构函数
        ~any()
        {
            delete ptr_;
        }

        // 检查是否包含值
        bool empty() const noexcept
        {
            return ptr_ == nullptr;
        }

        // 清除内容
        void reset() noexcept
        {
            delete ptr_;
            ptr_ = nullptr;
        }

        // 获取存储值的类型信息
        const std::type_info &type() const noexcept
        {
            return ptr_ ? ptr_->type() : typeid(void);
        }

        // 复制和赋值运算符
        any(const any &other) : ptr_(other.ptr_ ? other.ptr_->clone() : nullptr) {}

        any &operator=(const any &other)
        {
            if (this != &other)
            {
                delete ptr_;
                ptr_ = other.ptr_ ? other.ptr_->clone() : nullptr;
            }
            return *this;
        }

        any &operator=(any &&other) noexcept
        {
            if (this != &other)
            {
                delete ptr_;
                ptr_ = other.ptr_;
                other.ptr_ = nullptr;
            }
            return *this;
        }

        // 类型安全的值获取
        template <typename T>
        const T &get() const
        {
            if (type() != typeid(T))
            {
                throw std::bad_cast();
            }
            return static_cast<holder<T> *>(ptr_)->value;
        }

        template <typename T>
        T &get()
        {
            return const_cast<T &>(static_cast<const any &>(*this).get<T>());
        }

    private:
        // 基础容器基类
        struct placeholder
        {
            virtual ~placeholder() {}
            virtual const std::type_info &type() const = 0;
            virtual placeholder *clone() const = 0;
        };

        // 具体容器类型
        template <typename T>
        struct holder : public placeholder
        {
            holder(const T &val) : value(val) {}

            virtual const std::type_info &type() const override
            {
                return typeid(T);
            }

            virtual placeholder *clone() const override
            {
                return new holder(value);
            }

            T value;
        };

        placeholder *ptr_;
    };

    // any_cast 函数 - 模仿 boost::any_cast
    template <typename T>
    T any_cast(const any &a)
    {
        return a.get<T>();
    }

    template <typename T>
    T any_cast(any &a)
    {
        return a.get<T>();
    }

    template <typename T>
    T any_cast(any &&a)
    {
        return a.get<T>();
    }
}

#endif

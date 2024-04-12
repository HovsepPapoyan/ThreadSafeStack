/**
 * @file ThreadSafeStack.h
 *
 * @brief ThreadSafeStack class is an implementation of thread-safe stack adapter with strong exception safe guarantee.
 *
 * @author Hovsep Papoyan
 * Contact: papoyanhovsep93@gmail.com
 * @Date 2024-04-12
 *
 */

#ifndef THREAD_SAFE_STACK_H
#define THREAD_SAFE_STACK_H

#include <condition_variable>
#include <mutex>
#include <stack>

namespace mt
{
    template<typename AdaptElem,
        template<typename, typename> typename Cont = std::deque, typename ContElem = AdaptElem,
        template<typename> typename Alloc = std::allocator, typename AllocElem = ContElem>
    class ThreadSafeStack
    {
        std::stack<std::shared_ptr<AdaptElem>,
            Cont<std::shared_ptr<ContElem>,
            Alloc<std::shared_ptr<AllocElem>>>> m_stack;
        mutable std::mutex m_mutex;
        std::condition_variable m_condVar;

    public:
        ThreadSafeStack() = default;
        ~ThreadSafeStack() = default;
        ThreadSafeStack(const ThreadSafeStack& rhs);
        ThreadSafeStack(ThreadSafeStack&& rhs);
        ThreadSafeStack& operator=(const ThreadSafeStack& rhs);
        ThreadSafeStack& operator=(ThreadSafeStack&& rhs);
        void swap(ThreadSafeStack& rhs);

        void push(AdaptElem elem);
        void waitAndPop(AdaptElem& elem);
        std::shared_ptr<AdaptElem> waitAndPop();
        bool tryPop(AdaptElem& elem);
        std::shared_ptr<AdaptElem> tryPop();
        std::shared_ptr<AdaptElem> pop();
        void pop(AdaptElem& elem);
    };

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    ThreadSafeStack<AdaptElem, Cont, ContElem, Alloc, AllocElem>::ThreadSafeStack(const ThreadSafeStack& rhs)
    {
        std::lock_guard<std::mutex> lock(rhs.m_mutex);
        m_stack = rhs.m_stack;
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    ThreadSafeStack<AdaptElem, Cont, ContElem, Alloc, AllocElem>::ThreadSafeStack(ThreadSafeStack&& rhs)
    {
        std::lock_guard<std::mutex> lock(rhs.m_mutex);
        m_stack = std::move(rhs.m_stack);
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    ThreadSafeStack<AdaptElem, Cont, ContElem, Alloc, AllocElem>&
        ThreadSafeStack<AdaptElem, Cont, ContElem, Alloc, AllocElem>::operator=(const ThreadSafeStack& rhs)
    {
        if (this != &rhs)
        {
            std::scoped_lock lock(m_mutex, rhs.m_mutex);
            m_stack = rhs.m_stack;
        }
        return *this;
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    ThreadSafeStack<AdaptElem, Cont, ContElem, Alloc, AllocElem>&
        ThreadSafeStack<AdaptElem, Cont, ContElem, Alloc, AllocElem>::operator=(ThreadSafeStack&& rhs)
    {
        if (this != &rhs)
        {
            std::scoped_lock lock(m_mutex, rhs.m_mutex);
            m_stack = std::move(rhs.m_stack);
        }
        return *this;
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    void
        ThreadSafeStack<AdaptElem, Cont, ContElem, Alloc, AllocElem>::swap(ThreadSafeStack& rhs)
    {
        if (this != &rhs)
        {
            std::scoped_lock lock(m_mutex, rhs.m_mutex);
            std::swap(m_stack, rhs.m_stack);
        }
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    void
        ThreadSafeStack<AdaptElem, Cont, ContElem, Alloc, AllocElem>::push(AdaptElem elem)
    {
        std::shared_ptr<AdaptElem> data(std::make_shared<AdaptElem>(std::move(elem)));
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stack.push(std::move(data));
        }
        m_condVar.notify_one();
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    void
        ThreadSafeStack<AdaptElem, Cont, ContElem, Alloc, AllocElem>::waitAndPop(AdaptElem& elem)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condVar.wait(lock, [this] { return !m_stack.empty(); });
        elem = std::move(*m_stack.top());
        m_stack.pop();
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    std::shared_ptr<AdaptElem>
        ThreadSafeStack<AdaptElem, Cont, ContElem, Alloc, AllocElem>::waitAndPop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condVar.wait(lock, [this] { return !m_stack.empty(); });
        std::shared_ptr<AdaptElem> res = std::move(m_stack.top());
        m_stack.pop();
        return res;
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    bool
        ThreadSafeStack<AdaptElem, Cont, ContElem, Alloc, AllocElem>::tryPop(AdaptElem& elem)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stack.empty())
        {
            return false;
        }
        elem = std::move(*m_stack.top());
        m_stack.pop();
        return true;
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    std::shared_ptr<AdaptElem>
        ThreadSafeStack<AdaptElem, Cont, ContElem, Alloc, AllocElem>::tryPop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stack.empty())
        {
            return std::shared_ptr<AdaptElem>{};
        }
        std::shared_ptr<AdaptElem> res = std::move(m_stack.top());
        m_stack.pop();
        return res;
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    std::shared_ptr<AdaptElem>
        ThreadSafeStack<AdaptElem, Cont, ContElem, Alloc, AllocElem>::pop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stack.empty())
        {
            throw std::runtime_error("The ThreadSafeStack is empty");
        }
        std::shared_ptr<AdaptElem> res = std::move(m_stack.top());
        m_stack.pop();
        return res;
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    void
        ThreadSafeStack<AdaptElem, Cont, ContElem, Alloc, AllocElem>::pop(AdaptElem& elem)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stack.empty())
        {
            throw std::runtime_error("The ThreadSafeStack is empty");
        }
        elem = std::move(*m_stack.top());
        m_stack.pop();
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    void
        swap(ThreadSafeStack<AdaptElem, Cont, ContElem, Alloc, AllocElem>& lhs, ThreadSafeStack<AdaptElem, Cont, ContElem, Alloc, AllocElem>& rhs)
    {
        lhs.swap(rhs);
    }
} // namespace mt

#endif
